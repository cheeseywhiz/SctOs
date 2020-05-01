/* copied from xv6 */
/* Routines to let C code use special x86 instructions. */
#include <stddef.h>

static inline void
stosb(void *addr, int data, size_t cnt)
{
    __asm volatile(
        "cld; rep stosb" :
        "=D" (addr), "=c" (cnt) :
        "0" (addr), "1" (cnt), "a" (data) :
        "memory", "cc");
}

static inline void
stosl(void *addr, int data, size_t cnt)
{
    __asm volatile(
        "cld; rep stosl" :
        "=D" (addr), "=c" (cnt) :
        "0" (addr), "1" (cnt), "a" (data) :
        "memory", "cc");
}
