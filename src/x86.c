/* this module provides kernel routines that are close to the x86-64 cpu */
#include <stdint.h>
#include "opsys/x86.h"
#include "opsys/bootloader_data.h"
#include "opsys/virtual-memory.h"
#include "gdt.h"
#include "stubs.h"
#include "interrupts.h"
#include "x86.h"

struct x86_64_cpu cpu;

static void init_apic(void);
/* alignment not necessary, but it is a page-sized table */
static idt_t idt __aligned(PAGE_SIZE);
static void init_idt(void);

void
init_cpu(void)
{
    set_gdt(gdt, gdt_length);
    init_segment_selectors(GDTI_KERNEL_DATA, GDTI_KERNEL_CODE);
    init_idt();
    set_idt(idt);
    init_apic();
}

static void
init_apic(void)
{
    uint64_t apic_base = read_msr(IA32_APIC_BASE);
    uint64_t base_addr = apic_base & APIC_BASE_MASK;
    struct cpuid version;
    cpuid(CPUID_VERSION, &version);
    cpu.apic.paddr = base_addr;
    cpu.apic.vaddr = bootloader_data->mmio_base - PAGE_SIZE;
    cpu.apic.id = (uint8_t)(version.b >> 24);
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
