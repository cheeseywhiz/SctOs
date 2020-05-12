#pragma once
#define ARRAY_LENGTH(arr) ((sizeof(arr))/(sizeof(*arr)))
#define __weak __attribute__((weak))
#define __unused __attribute__((unused))
#define __malloc __attribute__((malloc))
#define __noreturn __attribute__((noreturn))
#define IN_RANGE(base, size, x) ((base) <= (x) && (x) < (base) + (size))

