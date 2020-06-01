/* this module provides functionality to handle x86 interrupts */
#include "opsys/x86.h"
#include "opsys/virtual-memory.h"
#include "util.h"
#include "interrupts.h"
#include "gdt.h"

/* defined in gen/vectors.S */
extern uint64_t vector_table[N_INTERRUPTS];
/* alignment not necessary, but it is a page-sized table */
static idt_t idt __aligned(PAGE_SIZE);

/* initialize the idt and load it into the idtr */
void
install_idt(void)
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

    set_idt(idt);
}

void
interrupt_handler(struct interrupt_frame *frame __unused)
{
}
