#pragma once
#define ARRAY_LENGTH(arr) ((sizeof(arr))/(sizeof(*arr)))
#define __weak __attribute__((weak))
#define __unused __attribute__((unused))
#define __malloc __attribute__((malloc))
#define __noreturn __attribute__((noreturn))
#define __section(sec) __attribute__((section(sec)))
#define IN_RANGE(base, size, x) ((base) <= (x) && (x) < (base) + (size))
#define HANG() do {} while (1)

void __builtin_unreachable(void);

/* set a debugging breakpoint */
void break_(void);

#if defined(_EFI_DEBUG) || defined(_KERNEL_DEBUG)
# define BREAK() break_()
#else
# define BREAK()
#endif
