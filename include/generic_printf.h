#pragma once
#include <stddef.h>

typedef void (*write_func)(const char*, size_t);

void generic_printf(write_func write, const char *format, ...);
