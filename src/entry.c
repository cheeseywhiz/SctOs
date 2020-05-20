#include "x86.h"

void main(void);

/* uefi enters _start in long mode with the kernel mapped to the higher half. */
void main(void)
{
    halt();
}
