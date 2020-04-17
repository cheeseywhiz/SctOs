#pragma once
#include <stddef.h>
#include <stdint.h>

enum vga_color {
    /* fg/bg */
    VGA_COLOR_BLACK,
    VGA_COLOR_BLUE,
    VGA_COLOR_GREEN,
    VGA_COLOR_CYAN,
    VGA_COLOR_RED,
    VGA_COLOR_MAGENTA,
    VGA_COLOR_BROWN,
    VGA_COLOR_LIGHT_GRAY,
    /* fg only */
    VGA_COLOR_DARK_GRAY,
    VGA_COLOR_LIGHT_BLUE,
    VGA_COLOR_LIGHT_GREEN,
    VGA_COLOR_LIGHT_CYAN,
    VGA_COLOR_LIGHT_RED,
    VGA_COLOR_LIGHT_MAGENTA,
    VGA_COLOR_LIGHT_BROWN,
    VGA_COLOR_WHITE,
};

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
static volatile uint16_t *const vga_port = (volatile uint16_t*)0xB8000;

void vga_scroll(size_t n);

static inline uint8_t
vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return bg << 4 | fg;
}

static inline uint16_t
vga_entry(unsigned char uc, uint8_t entry_color) {
    return ((uint16_t)entry_color << 8) | (uint16_t)uc;
}

static inline volatile uint16_t*
vga_at(size_t x, size_t y) {
    const size_t index = y * VGA_WIDTH + x;
    return &vga_port[index];
}

static inline void
vga_write(char c, uint8_t color, size_t x, size_t y) {
    *vga_at(x, y) = vga_entry(c, color);
}
