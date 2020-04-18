#include "generic_printf.h"
#include "string.h"
#include <stdarg.h>
#include <stdbool.h>

static void put_int(write_func, long long);
static void put_unsigned(write_func, unsigned long long);
static void put_hex(write_func, unsigned);
static void put_ul_hex(write_func, size_t);
static void putc(write_func, char);
static void puts(write_func, const char*);

static const char *const digits = "fedcba9876543210123456789abcdef";
#define GET_DIGIT(n, base) (digits[(n) % (base) + 15])

void
generic_printf(write_func write, const char *format, ...) {
    const char *first, *last;
    va_list ap;
    bool format_mode = false;
    int n_long = 0;
    va_start(ap, format);

    for (first = last = format; *last; ++last) {
        if (*last == '%') {
            if (format_mode)
                first = last;
            else
                write(first, last - first);

            format_mode = !format_mode;
            continue;
        } else if (!format_mode) {
            continue;
        }

        first = last + 1;
        bool reset = false;

        switch (*last) {
        case 'l':
            ++n_long;
            break;
        case 'd':
            switch(n_long) {
            case 0:
                put_int(write, va_arg(ap, int));
                break;
            case 1:
                put_int(write, va_arg(ap, long));
                break;
            case 2:
                put_int(write, va_arg(ap, long long));
                break;
            }

            reset = true;
            break;
        case 'u':
            switch (n_long) {
            case 0:
                put_unsigned(write, va_arg(ap, unsigned));
                break;
            case 1:
                put_unsigned(write, va_arg(ap, unsigned long));
                break;
            case 2:
                put_unsigned(write, va_arg(ap, unsigned long long));
                break;
            }

            reset = true;
            break;
        case 's':
            puts(write, va_arg(ap, const char*));
            reset = true;
            break;
        case 'c':
            putc(write, va_arg(ap, int));
            reset = true;
            break;
        case 'x':
            put_hex(write, va_arg(ap, unsigned));
            reset = true;
            break;
        case 'p':
            put_ul_hex(write, va_arg(ap, size_t));
            reset = true;
            break;
        }

        if (reset) {
            format_mode = false;
            n_long = 0;
        }
    }

    write(first, last - first);
    va_end(ap);
}

#ifndef ARRAY_LENGTH
#define ARRAY_LENGTH(arr) ((sizeof(arr)) / (sizeof(arr[0])))
#endif

/*
 * long long is the longest c signed int
 * and it's at most 20 characters long
 */
static void
put_int(write_func write, long long n) {
    char mem[21], *stack = mem + ARRAY_LENGTH(mem) - 1;
    int copy = n;
    *stack = 0;

    do {
        *--stack = GET_DIGIT(n, 10);
    } while (n /= 10);

    if (copy < 0)
        *--stack = '-';
    write(stack, strlen(stack));
}

static void
put_unsigned(write_func write, unsigned long long n) {
    char mem[21], *stack = mem + ARRAY_LENGTH(mem) - 1;
    *stack = 0;

    do {
        *--stack = GET_DIGIT(n, 10);
    } while (n /= 10);

    write(stack, strlen(stack));
}

static void
put_hex(write_func write, unsigned n) {
    char mem[] = "0x00000000";
    char *stack = mem + ARRAY_LENGTH(mem) - 1;

    do {
        *--stack = GET_DIGIT(n, 16);
    } while (n /= 16);

    write(mem, strlen(mem));
}

static void
put_ul_hex(write_func write, size_t n) {
    char mem[] = "0x0000000000000000";
    char *stack = mem + ARRAY_LENGTH(mem) - 1;

    do {
        *--stack = GET_DIGIT(n, 16);
    } while (n /= 16);

    write(mem, strlen(mem));
}

static void
putc(write_func write, char c) {
    write(&c, 1);
}

static void
puts(write_func write, const char *s) {
    if (!s)
        s = "NULL";
    write(s, strlen(s));
}
