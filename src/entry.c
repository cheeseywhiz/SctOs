#include <stddef.h>
#include "string.h"
#include "opsys/x86.h"
#include "opsys/bootloader_data.h"
#include "opsys/kernel_main.h"

/* set during kernel boot. */
struct bootloader_data *bootloader_data __section(".data.rel.ro");
/* linker script variables */
extern char __bss_start[], _end[];

/* uefi enters _start in long mode with the kernel mapped to the higher half and
 * bootloader_data mapped to the physical memory region. */
void kernel_main(struct bootloader_data *bootloader_data_in)
{
    BREAK();
    /* clear bss */
    memset(__bss_start, 0, (size_t)_end - (size_t)__bss_start);
    bootloader_data = bootloader_data_in;
    halt();
}
