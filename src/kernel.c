#include "terminal.h"
#include "vga.h"
#include <stddef.h>
#include <stdint.h>

#if defined(__linux__) || !defined(__i386__)
# error "You are not using the cross compiler"
#endif

void kernel_main(void);

void
kernel_main(void) {
    terminal_clear();

    /*
     * with adding one it says goodbye at the top,
     * without it says hello at the top,
     * which should indicate scroll is working
     */
    for (int i = 0; i < VGA_HEIGHT + 1; ++i) {
        if (i % 2)
            tprintf("Hello, kernel world!\tkernel_main = \t%p;\n",
                    kernel_main);
        else
            tprintf("Goodbye, kernel world!\tkprintf = \t%p;\n",
                    generic_printf);
    }

    tprintf("%lu\n", (size_t)kernel_main);
    tprintf("%ld %ld\n", INT32_MAX, INT32_MIN);
}
