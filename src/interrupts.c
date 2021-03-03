/* this module provides functionality to handle x86 interrupts */
#include "opsys/x86.h"
#include "opsys/virtual-memory.h"
#include "util.h"
#include "interrupts.h"
#include "gdt.h"

/* 
 * it has global linkage so the stub can pass in the magic.
 * XXX: initialize this to a random number from the bootloader?
 * it's set to a frowny face because thats what you see when you mess up.
 */
const uint64_t interrupt_magic = 0xD1D1D1D1D1D1D1D1;

void
interrupt_handler(struct interrupt_frame *frame, uint64_t magic)
{
    /* to catch when gdb jumps here for odd reasons */
    if (magic != interrupt_magic)
        halt();

    switch (frame->interrupt_number) {
    case EXC_BP:
        return;
    default:
        BREAK();
        break;
    }
}
