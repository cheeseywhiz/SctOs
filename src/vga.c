#include "vga.h"

/* copy the bottom height - n lines to the top */
void
vga_scroll(size_t n) {
    for (size_t y = n; y < VGA_HEIGHT; ++y) {
        for (size_t x = 0; x < VGA_WIDTH; ++x)
            *VGA_AT(x, y - n) = *VGA_AT(x, y);
    }
}
