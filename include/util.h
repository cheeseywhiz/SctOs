#pragma once
#include <stddef.h>
#define ARRAY_LENGTH(arr) ((sizeof(arr))/(sizeof(*arr)))
#define __weak __attribute__((weak))
#define __unused __attribute__((unused))
#define __malloc __attribute__((malloc))
#define __noreturn __attribute__((noreturn))
#define __packed __attribute__((packed))
#define __section(sec) __attribute__((section(sec)))
#define __ro_after_init __section(".data.rel.ro")
#define IN_RANGE(base, size, x) ((base) <= (x) && (x) < (base) + (size))
#define HANG() do {} while (1)

void __builtin_unreachable(void);

/* set a debugging breakpoint */
void break_(void);

#if defined(_EFI_DEBUG) || defined(_KERNEL_DEBUG)
# define BREAK() do { break_(); } while (0);
#else
# define BREAK() do {} while (0);
#endif

/* required to be defined per osdev "Meaty Skeleton" */
void *memcpy(void *dst, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
void *memmove(void *dst, const void *src, size_t n);
void* memset(void *s, int c, size_t n);
