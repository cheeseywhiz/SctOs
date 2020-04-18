#include "string.h"
#include "terminal.h"
#include <stdarg.h>
#include <stdbool.h>

static void kput_int(long long);
static void kput_unsigned(unsigned long long);
static void kput_hex(unsigned);
static void kput_ul_hex(size_t);
static void kputs(const char*);

void
kprintf(const char *format, ...) {
    const char *first, *last;
    bool format_mode = false;
    int n_long = 0;
    va_list ap;
    va_start(ap, format);

    for (first = last = format; *last; ++last) {
        if (*last == '%') {
            if (format_mode)
                first = last;
            else
                terminal_write(first, last - first);

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
                kput_int(va_arg(ap, int));
                break;
            case 1:
                kput_int(va_arg(ap, long));
                break;
            case 2:
                kput_int(va_arg(ap, long long));
                break;
            }

            reset = true;
            break;
        case 'u':
            switch (n_long) {
            case 0:
                kput_unsigned(va_arg(ap, unsigned));
                break;
            case 1:
                kput_unsigned(va_arg(ap, unsigned long));
                break;
            case 2:
                kput_unsigned(va_arg(ap, unsigned long long));
                break;
            }

            reset = true;
            break;
        case 's':
            kputs(va_arg(ap, const char*));
            reset = true;
            break;
        case 'c':
            terminal_putchar(va_arg(ap, int));
            reset = true;
            break;
        case 'x':
            kput_hex(va_arg(ap, unsigned));
            reset = true;
            break;
        case 'p':
            kput_ul_hex(va_arg(ap, size_t));
            reset = true;
            break;
        }

        if (reset) {
            format_mode = false;
            n_long = 0;
        }
    }

    terminal_write(first, last - first);
    va_end(ap);
}

#ifndef ARRAY_LENGTH
#define ARRAY_LENGTH(arr) ((sizeof(arr))/(sizeof(arr[0])))
#endif

/*
 * long long is the longest c signed int
 * and it's at most 20 characters long
 */
static void
kput_int(long long n) {
    char mem[21], *stack = mem + ARRAY_LENGTH(mem) - 1;
    int copy = n;
    *stack = 0;

    do {
        char c = n % 10;

        if (n > 0)
            c += '0';
        else
            c = '0' - c;

        *--stack = c;
    } while (n /= 10);

    if (copy < 0)
        *--stack = '-';
    terminal_writestring(stack);
}

static void
kput_unsigned(unsigned long long n) {
    char mem[21], *stack = mem + ARRAY_LENGTH(mem) - 1;
    *stack = 0;

    do {
        *--stack = (n % 10) + '0';
    } while (n /= 10);

    terminal_writestring(stack);
}

static void
kput_hex(unsigned n) {
    char mem[] = "0x00000000";
    char *stack = mem + ARRAY_LENGTH(mem) - 1;

    do {
        char c = n % 16;

        if (c < 10)
            c += '0';
        else
            c += 'a' - 10;

        *--stack = c;
    } while (n /= 16);

    terminal_writestring(mem);
}

static void
kput_ul_hex(size_t n) {
    char mem[] = "0x0000000000000000";
    char *stack = mem + ARRAY_LENGTH(mem) - 1;

    do {
        char c = n % 16;

        if (c < 10)
            c += '0';
        else
            c += 'a' - 10;

        *--stack = c;
    } while (n /= 16);

    terminal_writestring(mem);
}

static void
kputs(const char *s) {
    if (!s)
        s = "NULL";
    terminal_writestring(s);
}
