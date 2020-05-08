#pragma once
#include <stddef.h>

typedef void (*write_func)(const char*, size_t);

/* printf can handle %, l, u, s, c, p, x */
void generic_printf(void (*write_func)(const char*, size_t),
                    const char *format, ...)
    __attribute__((format(printf, 2, 3)));
