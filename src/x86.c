/* this module provides kernel routines that are close to the x86-64 cpu */
#include <stdint.h>
#include <stdbool.h>
#include "opsys/x86.h"
#include "opsys/bootloader_data.h"
#include "opsys/virtual-memory.h"
#include "util.h"
#include "gdt.h"
#include "stubs.h"
#include "interrupts.h"
#include "x86.h"

struct x86_64_cpu cpu;

/* alignment not necessary, but it is a page-sized table */
static idt_t idt __aligned(PAGE_SIZE);
static void init_idt(void);

/* x86-64-system table 11-11 */
static ia32_pat_t page_attribute_table = { .t = {
    /* modified table 11-12 to not use UC-, since we don't own the MTRRs yet */
    { PA_WB, 0 }, /* !AT !CD !WT */
    { PA_WT, 0 }, /* !AT !CD  WT */
    { PA_UC, 0 }, /* !AT  CD !WT */
    { PA_UC, 0 }, /* !AT  CD  WT */
    { PA_WB, 0 }, /*  AT !CD !WT */
    { PA_WT, 0 }, /*  AT !CD  WT */
    { PA_UC, 0 }, /*  AT  CD !WT */
    { PA_UC, 0 }, /*  AT  CD  WT */
} };

void
init_cpu(void)
{
    set_gdt(gdt, gdt_length);
    init_segment_selectors(GDTI_KERNEL_DATA, GDTI_KERNEL_CODE);
    init_idt();
    set_idt(idt);

    uint64_t pat_flat __unused = *(uint64_t*)&page_attribute_table;
    write_msr(IA32_PAT, pat_flat);

    /* set information for paging. the rest of apic initialization occurs after
     * paging is final. */
    uint64_t apic_base = read_msr(IA32_APIC_BASE);
    uint64_t base_addr = apic_base & APIC_BASE_MASK;
    cpu.apic.paddr = base_addr;
    cpu.apic.vaddr = bootloader_data->mmio_base - PAGE_SIZE;
}

/* defined in gen/vectors.S */
extern uint64_t vector_table[N_INTERRUPTS];

/* initialize the idt by corresponding each vector function to the same-numbered
 * table entry */
static void
init_idt(void)
{
    /* this function would be a good candidate for __attribute__((constructor))
     * since it's just doing math */
    for (uint16_t i = 0; i < N_INTERRUPTS; ++i) {
        uint64_t vector = vector_table[i];
        idt_entry_t *ent = &idt[i];
        /* x86-64-system figure 6-7 */
        /* set offset */
        (*ent)[1] = (vector & 0xffffffff00000000) >> 32;
        (*ent)[0] = ((vector & 0xffff0000) << 32) | (vector & 0xffff);
        /* set code segment selector */
        uint16_t code_segment_selector = GDTI_KERNEL_CODE << 3;
        (*ent)[0] |= (uint64_t)(code_segment_selector << 16);
        /* present, callable by ring 0, interrupt gate */
        (*ent)[0] |= SEGDESC_P | SEGDESC_SET_DPL(0) | SDT_INTR;
    }
}

static void init_apic_impl(void);

static void
init_apic_impl(void)
{
    /* verify that apic may be used */
    struct cpuid version;
    cpuid(CPUID_VERSION, &version);
    bool pat_exists = version.a & CPUID_PAT;
    bool x2apic_exists = version.c & CPUID_x2APIC;
    bool apic_exists = version.d & CPUID_APIC;
    if (!pat_exists || !apic_exists || !x2apic_exists)
        halt();

    ia32_pat_t pat __unused = { .n = read_msr(IA32_PAT) };

    /* XXX: why does this memory access cause gdb to lose single step? */
    BREAK();
    uint32_t lapic_version = *(uint32_t volatile*)(cpu.apic.vaddr + 0x30);
    BREAK();
    uint8_t lapic_version_num = (uint8_t)lapic_version;
    /* versions outside of this range do not correspond to apic existence */
    if (!IN_RANGE(0x10, 0x15 + 1, lapic_version_num))
        halt();

    uint8_t max_lvt __unused = (uint8_t)(lapic_version >> 16);
    bool can_suppress_eoi __unused = lapic_version & (1 << 24);
}

__stack_protector
void
init_apic(void)
{
    init_apic_impl();
}
