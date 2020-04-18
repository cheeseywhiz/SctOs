#pragma once
#include "generic_printf.h"
#include <stddef.h>
#include <stdint.h>

extern size_t terminal_row;
extern size_t terminal_column;
extern uint8_t terminal_color;

void terminal_clear(void);
void terminal_write(const char *data, size_t size);
#define tprintf(format, ...) generic_printf(terminal_write, format, __VA_ARGS__)
