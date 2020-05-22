/* x86.h functions that can't be inlined go here */
#include "opsys/x86.h"
#include "util.h"

void
halt(void)
{
    disable_interrupts();
    __asm volatile("hlt");
    HANG();
}
