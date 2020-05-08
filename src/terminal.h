#pragma once
#include "generic_printf.h"
#include <stddef.h>
#include <stdint.h>

/* the vertical position of the cursor */
extern size_t terminal_row;
/* the horizontal position of the cursor */
extern size_t terminal_column;
extern uint8_t terminal_fg_color;
extern uint8_t terminal_bg_color;

void terminal_clear(void);
/* write the given string to the terminal */
void terminal_write(const char *data, size_t size);
/* write formatted output to the terminal */
#define tprintf(format, ...) generic_printf(terminal_write, format, __VA_ARGS__)
