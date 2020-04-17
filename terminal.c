#include "terminal.h"
size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;

void
terminal_initialize(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GRAY, VGA_COLOR_BLACK);

    for (size_t y = 0; y < VGA_HEIGHT; ++y) {
        for (size_t x = 0; x < VGA_WIDTH; ++x)
            terminal_clear_char(x, y);
    }
}

void
terminal_scroll(size_t n) {
    vga_scroll(n);

    for (size_t y = VGA_HEIGHT - n; y < VGA_HEIGHT; ++y) {
        for (size_t x = 0; x < VGA_WIDTH; ++x)
            terminal_clear_char(x, y);
    }
}

void
terminal_write(const char *data, size_t size) {
    for (size_t i = 0; i < size; ++i)
        terminal_putchar(data[i]);
}

