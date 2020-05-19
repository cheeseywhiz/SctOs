/* copied from xv6 */
/* Routines to let C code use special x86 instructions. */
#include <stddef.h>
#include "util.h"

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

static inline void
disable_interrupts(void)
{
    __asm volatile("cli");
}

static inline __noreturn void
halt(void)
{
    disable_interrupts();
    __asm volatile("hlt");
    HANG();
}
