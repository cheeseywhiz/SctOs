#include "terminal.h"
#include "vga.h"
size_t terminal_row = 0;
size_t terminal_column = 0;
uint8_t terminal_color = VGA_ENTRY_COLOR(VGA_COLOR_WHITE,
                                         VGA_COLOR_BLUE);
#define CLEAR_CHAR(x, y) VGA_WRITE(' ', terminal_color, x, y)

static void putc(char);
static void inc_column(size_t);
static void inc_row(size_t);
static void scroll(size_t);

static const int tab_size = 8;

void
terminal_clear(void) {
    for (size_t y = 0; y < VGA_HEIGHT; ++y) {
        for (size_t x = 0; x < VGA_WIDTH; ++x)
            CLEAR_CHAR(x, y);
    }
}

void
terminal_write(const char *data, size_t size) {
    for (size_t i = 0; i < size; ++i)
        putc(data[i]);
}

static void
putc(char c) {
    if (c == '\n') {
        inc_row(1);
        terminal_column = 0;
    } else if (c == '\t') {
        do {
            inc_column(1);
        } while (terminal_column % tab_size);
    } else {
        VGA_WRITE(c, terminal_color, terminal_column, terminal_row);
        inc_column(1);
    }
}

static void
inc_column(size_t n) {
    terminal_column += n;

    if (terminal_column >= VGA_WIDTH) {
        inc_row(1);
        terminal_column = 0;
    }
}

static void
inc_row(size_t n) {
    terminal_row += n;

    if (terminal_row >= VGA_HEIGHT) {
        scroll(terminal_row - VGA_HEIGHT + 1);
        terminal_row = VGA_HEIGHT - 1;
    }
}

static void
scroll(size_t n) {
    vga_scroll(n);

    for (size_t y = VGA_HEIGHT - n; y < VGA_HEIGHT; ++y) {
        for (size_t x = 0; x < VGA_WIDTH; ++x)
            CLEAR_CHAR(x, y);
    }
}
