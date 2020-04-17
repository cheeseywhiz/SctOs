#include "vga.h"
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

    /* with adding one it says goodbye at the top,
     * without it says hello at the top,
     * which should indicate scroll is working
     */
    for (int i = 0; i < VGA_HEIGHT + 1; ++i) {
        if (i % 2)
            terminal_writestring("Hello, kernel world!\n");
        else
            terminal_writestring("Goodbye, kernel world!\n");
    }
}
