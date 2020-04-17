#include "vga.h"

void
vga_scroll(size_t n) {
    for (size_t y = n; y < VGA_HEIGHT; ++y) {
        for (size_t x = 0; x < VGA_WIDTH; ++x) {
            *vga_at(x, y - n) = *vga_at(x, y);
        }
    }
}
