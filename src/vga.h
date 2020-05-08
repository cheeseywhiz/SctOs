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
static volatile uint16_t *const vga_port = (volatile uint16_t*)0xC03FF000;
#define _ENTRY(c, fg, bg) ((uint16_t)(((bg) << 12) | ((fg) << 8) | (c)))
#define VGA_AT(x, y) (&vga_port[(y) * VGA_WIDTH + (x)])
#define VGA_WRITE(x, y, c, fg, bg) (*VGA_AT(x, y) = _ENTRY(c, fg, bg))

/* copy the bottom height - n lines to the top */
void vga_scroll(size_t n);
