#pragma once
#include "string.h"
#include "vga.h"
#include <stddef.h>
#include <stdint.h>

extern size_t terminal_row;
extern size_t terminal_column;
extern uint8_t terminal_color;

void terminal_initialize(void);
void terminal_scroll(size_t n);
void terminal_write(const char *data, size_t size);

static inline void
terminal_writestring(const char *data) {
    terminal_write(data, strlen(data));
}

static inline void
terminal_clear_char(size_t x, size_t y) {
    vga_write(' ', terminal_color, x, y);
}

static inline void
terminal_inc_row(size_t n) {
    terminal_row += n;

    if (terminal_row >= VGA_HEIGHT) {
        terminal_scroll(terminal_row - VGA_HEIGHT + 1);
        terminal_row = VGA_HEIGHT - 1;
    }
}

static inline void
terminal_inc_column(size_t n) {
    terminal_column += n;

    if (terminal_column >= VGA_WIDTH) {
        terminal_inc_row(1);
        terminal_column = 0;
    }
}

static inline void
terminal_putchar(char c) {
    if (c == '\n') {
        terminal_inc_row(1);
        terminal_column = 0;
    } else if (c == '\t') {
        do {
            terminal_inc_column(1);
        } while (terminal_column % 8);
    } else {
        vga_write(c, terminal_color, terminal_column, terminal_row);
        terminal_inc_column(1);
    }
}
