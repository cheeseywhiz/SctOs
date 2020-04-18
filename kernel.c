#include "string.h"
#include "terminal.h"
#include <stddef.h>
#include <stdint.h>

#if defined(__linux__) || !defined(__i386__)
# error "You are not using the cross compiler"
#endif

void
kernel_main(void) {
    terminal_initialize();

    /*
     * with adding one it says goodbye at the top,
     * without it says hello at the top,
     * which should indicate scroll is working
     */
    for (int i = 0; i < VGA_HEIGHT + 1; ++i) {
        if (i % 2)
            kprintf("Hello, kernel world!\tkernel_main = \t%p;\n",
                    kernel_main);
        else
            kprintf("Goodbye, kernel world!\tkprintf = \t%p;\n",
                    kprintf);
    }

    kprintf("%lu\n", (size_t)kernel_main);
    kprintf("%ld %ld\n", INT32_MAX, INT32_MIN);
}
