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
#define VGA_ENTRY_COLOR(fg, bg) ((bg) << 4 | (fg))
#define VGA_ENTRY(c, entry_color) (((uint16_t)(entry_color) << 8) | (uint16_t)(c))
#define VGA_AT(x, y) (&vga_port[(y) * VGA_WIDTH + (x)])
#define VGA_WRITE(c, entry_color, x, y) (*VGA_AT(x, y) = VGA_ENTRY(c, entry_color))

void vga_scroll(size_t n);
