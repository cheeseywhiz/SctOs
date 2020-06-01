/* this module provides the kernel entry point */
#include <stddef.h>
#include "string.h"
#include "opsys/x86.h"
#include "opsys/bootloader_data.h"
#include "opsys/kernel_main.h"
#include "opsys/virtual-memory.h"
#include "virtual-memory.h"
#include "stubs.h"
#include "interrupts.h"
#include "gdt.h"

/* set during kernel boot. */
struct bootloader_data *bootloader_data __ro_after_init;
/* linker script variables */
extern char __bss_start[], _end[];
static __noreturn main2_t main2;

/* uefi enters _start in long mode with the kernel mapped to the higher half and
 * bootloader_data mapped to the physical memory region. */
void kernel_main(struct bootloader_data *bootloader_data_in)
{
    /* clear bss */
    memset(__bss_start, 0, (size_t)_end - (size_t)__bss_start);
    bootloader_data = bootloader_data_in;

    /* start off with some initial memory */
    for (uint64_t i = 0; i < bootloader_data->n_pages; ++i)
        free_physical_page((void*)((uint64_t)bootloader_data->free_memory
                                             + PAGE_SIZE * i));

    void *new_stack;
    if (!(new_stack = allocate_physical_page(APP_NORMAL)))
        halt(); /* nomem */
    setup_new_stack(main2, new_stack);
    /* control transfers almost directly to main2 with new stack */
    __builtin_unreachable();
}

void main2(void)
{
    page_table_t *kernel_address_space = new_address_space();
    set_cr3((uint64_t)kernel_address_space - bootloader_data->paddr_base);
    set_gdt(gdt, gdt_length);
    init_segment_selectors(GDTI_KERNEL_DATA, GDTI_KERNEL_CODE);
    install_idt();
    interrupt(69);
    int3();
    BREAK();
    uefi_call_wrapper(bootloader_data->RT->ResetSystem, 4,
        EfiResetShutdown, EFI_SUCCESS, 0, NULL);
    __builtin_unreachable();
}
