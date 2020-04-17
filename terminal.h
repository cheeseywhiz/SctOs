#pragma once
#include "string.h"
#include "vga.h"
#include <stddef.h>
#include <stdint.h>

extern size_t terminal_row;
extern size_t terminal_column;
extern uint8_t terminal_color;

static inline void
terminal_initialize(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GRAY, VGA_COLOR_BLACK);

    for (size_t y = 0; y < VGA_HEIGHT; ++y) {
        for (size_t x = 0; x < VGA_WIDTH; ++x)
            vga_write(' ', terminal_color, x, y);
    }
}

static inline void
terminal_putchar(char c) {
    vga_write(c, terminal_color, terminal_column, terminal_row);

    if (++terminal_column == VGA_WIDTH) {
        terminal_column = 0;

        if (++terminal_row == VGA_HEIGHT)
            terminal_row = 0;
    }
}

static inline void
terminal_write(const char *data, size_t size) {
    for (size_t i = 0; i < size; ++i)
        terminal_putchar(data[i]);
}


static inline void
terminal_writestring(const char *data) {
    terminal_write(data, strlen(data));
}
