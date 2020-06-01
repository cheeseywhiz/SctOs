/* copied from xv6 */
/* Routines to let C code use special x86 instructions. */
#include <stddef.h>
#include <stdint.h>
#include "util.h"

static __always_inline void
pause(void)
{
    __asm volatile("pause");
}

static __always_inline void
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

/* x86-64-system S3.4.5 */
/* size of the segment */
#define SEGDESC_GET_LIMIT(d) \
    (uint32_t)((((d) & (0xfUL << 48)) >> 32) | \
               ((d) & 0xffff))
#define SEGDESC_SET_LIMIT(ll) \
    ((((ll) & (0xfUL << 16)) << 32) | \
     ((ll) & 0xffff))
/* base address of the segment */
#define SEGDESC_GET_BASE(d) \
    ((((d) & (0xffUL << 56)) >> 24) | \
     (((d) & (0xffffffUL << 16)) >> 16))
#define SEGDESC_SET_BASE(b) \
    ((((b) & (0xffUL << 24)) << 32) | \
     (((b) & 0xffffff) << 16))
/* descriptor privilege level */
#define SEGDESC_GET_DPL(d) \
    (((d) & (0x3UL << 45)) >> 45)
#define SEGDESC_SET_DPL(dpl) \
    (uint64_t)((uint64_t)((dpl) & 0x3) << 45)

#define SEGDESC_A   (1UL << 40) /* accessed (formally part of type) */
#define SEGDESC_S   (1UL << 44) /* true: code/data; false: system */
#define SEGDESC_P   (1UL << 47) /* present */
#define SEGDESC_AVL (1UL << 52) /* available for os use */
#define SEGDESC_L   (1UL << 53) /* true: 64-bit code segment */
#define SEGDESC_DB  (1UL << 54) /* D/B flag (varies) */
#define SEGDESC_D SEGDESC_DB
#define SEGDESC_B SEGDESC_DB
#define SEGDESC_G   (1UL << 55) /* true: 4kb, false: byte granularity */

/* code/data segment descriptor type */
/* if not SDT_X */
#define SDT_RW   (1UL << 41) /* true: read/write; false: read only */
#define SDT_DOWN (1UL << 42) /* true: expands down; false: expands up */
/* if SDT_X */
#define SDT_X    (1UL << 43) /* executable segment */
#define SDT_XR   (1UL << 41) /* true: executable/read; false: execute only */
#define SDT_CONF (1UL << 42) /* conforming (can jump into higher-privileged
                              * segment while maintaining current privilege
                              * level) */

/* system segment descriptor type */
#define SDT_MASK (0xfUL << 40)
#define SDT_LDT  (2UL << 40)  /* local descriptor table */
#define SDT_TSSA (9UL << 40)  /* available task segment selector */
#define SDT_TSSB (11UL << 40) /* busy task segment selector */
#define SDT_CALL (12UL << 40) /* call gate */
#define SDT_INTR (14UL << 40) /* interrupt gate */
#define SDT_TRAP (15UL << 40) /* trap gate */

/* x86-64-system figure 3-11 */
struct pseudo_descriptor {
    uint16_t limit;
    uint64_t *base;
} __packed;

static inline uint64_t*
get_gdt(uint16_t *length)
{
    struct pseudo_descriptor gdtr;
    __asm__ ("sgdt %0" : "=m"(gdtr));
    if (length)
        *length = (uint16_t)((size_t)(gdtr.limit + 1) / sizeof(*gdtr.base));
    return gdtr.base;
}

static inline void
set_gdt(uint64_t *gdt, uint16_t length)
{
    struct pseudo_descriptor gdtr;
    gdtr.limit = (uint16_t)((sizeof(*gdtr.base) * length) - 1);
    gdtr.base = gdt;
    __asm__ ("lgdt %0" :: "m"(gdtr));
}

#define N_INTERRUPTS 256
typedef uint64_t idt_entry_t[2];
typedef idt_entry_t idt_t[N_INTERRUPTS];

static inline void
set_idt(idt_t idt)
{
    struct pseudo_descriptor idtr;
    idtr.limit = sizeof(idt_t) - 1;
    idtr.base = (uint64_t*)idt;
    __asm__ __volatile__("lidt %0" :: "m"(idtr));
}

/* x86-64-system table 6-1 */
enum exception_number {
    EXC_DE, /* divide error */
    EXC_DB, /* debug exception */
    EXC_NMI, /* non-maskable external interrupt */
    EXC_BP, /* breakpoint */
    EXC_OF, /* overflow */
    EXC_BR, /* bound range exceeded */
    EXC_UD, /* undefined opcode */
    EXC_NM, /* no math coprocessor */
    EXC_DF, /* double fault */
    EXC_RES9, /* reserved */
    EXC_TS, /* invalid TSS */
    EXC_NP, /* segment not present */
    EXC_SS, /* stack segment fault */
    EXC_GP, /* general protection fault */
    EXC_PF, /* page fault */
    EXC_RES15, /* reserved */
    EXC_MF, /* math fault */
    EXC_AC, /* alignment check */
    EXC_MC, /* machine check */
    EXC_XM, /* SIMD floating point exception */
    EXC_VE, /* virtualization exception */
    EXC_RES21, /* reserved... */
    EXC_RES22,
    EXC_RES23,
    EXC_RES24,
    EXC_RES25,
    EXC_RES26,
    EXC_RES27,
    EXC_RES28,
    EXC_RES29,
    EXC_RES30,
    EXC_RES31,
};

/* interrupts >= 32 are user defined */
#define EXC_IS_EXCEPTION(num) ((num) < 32)
/* error codes refer to a descriptor, or an address for PF */
#define EXC_HAS_ERROR_CODE(num) \
    ((num) == EXC_DF || (num) == EXC_TS || (num) == EXC_NP || \
     (num) == EXC_SS || (num) == EXC_GP || (num) == EXC_PF || (num) == EXC_AC)

/* registers in the order of pushaq */
struct x86_64_registers {
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rbp;
    uint64_t rsp;
    uint64_t rbx;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rax;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
};

/* prepared by interrupt vector functions and interrupt handler stub */
struct interrupt_frame {
    struct x86_64_registers reg;
    uint64_t interrupt_number;
    uint64_t error_code;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};

static __always_inline void
interrupt(const uint8_t num)
{
    __asm__ __volatile__("int %0" :: "i"(num));
}

static __always_inline void
int3(void)
{
    __asm__ __volatile__("int3");
}
