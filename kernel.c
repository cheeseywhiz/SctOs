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
    terminal_writestring("Hello, kernel world!\n");
}
