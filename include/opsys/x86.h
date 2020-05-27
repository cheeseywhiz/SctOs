/* copied from xv6 */
/* Routines to let C code use special x86 instructions. */
#include <stddef.h>
#include <stdint.h>
#include "util.h"

__attribute__((always_inline))
static inline void
pause(void)
{
    __asm volatile("pause");
}

__attribute__((always_inline))
static inline void
noop(void)
{
    __asm volatile("nop");
}

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

__noreturn void halt(void);

/* x86-64-system figure 2-7 */
enum cr0_flags {
    CR0_PE = 1 <<  0, /* protected mode */
    CR0_MP = 1 <<  1, /* monitor coprocessor */
    CR0_EM = 1 <<  2, /* emulation */
    CR0_TS = 1 <<  3, /* task switched */
    CR0_ET = 1 <<  4, /* extension type */
    CR0_NE = 1 <<  5, /* numeric error reporting */
    CR0_WP = 1 << 16, /* write protect */
    CR0_AM = 1 << 18, /* alignment check enable */
    CR0_NW = 1 << 29, /* write-through disable */
    CR0_CD = 1 << 30, /* cache disable */
#define CR0_PG (1U << 31) /* paging */
};

static inline uint64_t
get_cr0(void)
{
    uint64_t cr0;
    __asm volatile("mov %%cr0, %0"
                   : "=r"(cr0));
    return cr0;
}

static inline uint64_t
set_cr0(uint64_t cr0)
{
    __asm volatile("mov %0, %%cr0"
                   :: "r"(cr0));
    return cr0;
}

/* x86-64-system: cr3 stores the physical address to the 4th level paging
 * structure. the last 12 bits (which are zero because of page alignment)
 * contain flags but they don't mean anything with 4-level paging, so these bits
 * should stay zeroed. */
static inline uint64_t
get_cr3(void)
{
    uint64_t cr3;
    __asm volatile("mov %%cr3, %0"
                   : "=r"(cr3));
    return cr3;
}

static inline uint64_t
set_cr3(uint64_t cr3)
{
    __asm volatile("mov %0, %%cr3"
                   :: "r"(cr3));
    return cr3;
}

/* x86-64-system figure 2-7 */
enum cr4_flags {
    CR4_VME  = 1 <<  0, /* virtual-8086 mode */
    CR4_PVI  = 1 <<  1, /* protected-mode virtual interrupts (S20.3) */
    CR4_TSD  = 1 <<  2, /* time stamp disable */
    CR4_DE   = 1 <<  3, /* debugging exceptions */
    CR4_PSE  = 1 <<  4, /* page size extensions
                         * true: 4MB pages, false: 4KB pages (32 bit addrs) */
    CR4_PAE  = 1 <<  5, /* physical address extension (to 64 bit addrs) */
    CR4_MCE  = 1 <<  6, /* machine-check enable */
    CR4_PGE  = 1 <<  7, /* page global enable */
    CR4_PCE  = 1 <<  8, /* performance-monitoring counter enable */
    CR4_OSFXSR = 1 << 9, /* enable FXSAVE & FXRSTOR instructions */
    CR4_OSXMMEXCPT = 1 << 10, /* enable unmasked SIMD floating point
                               * exceptions */
    CR4_UMIP = 1 << 11, /* user-mode instruction prevention */
    CR4_VMXE = 1 << 13, /* VMX enable */
    CR4_SMXE = 1 << 14, /* SMX enable */
    CR4_FSGSBASE = 1 << 16, /* enable several instructions */
    CR4_PCIDE = 1 << 17, /* PCID enable (S4.10.1) */
    CR4_OSXSAVE = 1 << 18, /* enable several instructions */
    CR4_SMEP = 1 << 20, /* supervisor-mode execution prevention */
    CR4_SMAP = 1 << 21, /* supervisor-mode access prevention */
    CR4_PKE  = 1 << 22, /* protection-key enable */
};

static inline uint64_t
get_cr4(void)
{
    uint64_t cr4;
    __asm volatile("mov %%cr4, %0"
                   : "=r"(cr4));
    return cr4;
}

static inline uint64_t
set_cr4(uint64_t cr4)
{
    __asm volatile("mov %0, %%cr4"
                   :: "r"(cr4));
    return cr4;
}

static inline uint64_t
read_msr(uint32_t which_msr)
{
    uint32_t eax, edx;
    __asm volatile(
        "rdmsr"
        : "=a"(eax), "=d"(edx)
        : "c"(which_msr));
    return ((uint64_t)edx << 32) | eax;
}

/* x86-64-msr page 2-45 */
#define IA32_EFER 0xc0000080
enum ia32_efer_flags {
    EFER_SYSCALL = 1 <<  0, /* enable syscall/sysret instructions */
    EFER_LME     = 1 <<  8, /* long mode enable */
    EFER_LMA     = 1 << 10, /* long mode active */
    EFER_NXE     = 1 << 11, /* NX bit enable */
};
