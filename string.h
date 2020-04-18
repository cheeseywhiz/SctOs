#pragma once
#include <stddef.h>

void kprintf(const char *format, ...);

static inline size_t
strlen(const char *str) {
    size_t len = 0;
    while (str[len])
        ++len;
    return len;
}
