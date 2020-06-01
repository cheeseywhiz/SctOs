#include "opsys/virtual-memory.h"
#include "asm-util.h"

.section .text

/* void setup_new_stack(void *func, void *new_stack_bottom); */
.global setup_new_stack
.type setup_new_stack, @function
setup_new_stack:
        lea     PAGE_SIZE(%rsi), %rsp
        xor     %rbp, %rbp
        push    %rbp
        jmp     *%rdi

/* void init_segment_selectors(uint16_t kdata, uint16_t kcode);
 * set ds, es, fs, gs, to null selector, ss to given data segment index in GDT,
 * cs to given code segment index in GDT */
.global init_segment_selectors
.type init_segment_selectors, @function
init_segment_selectors:
        /* set ds, es, fs, gs to null selector */
        xor     %ax, %ax        /* null segment */
        mov     %ax, %ds
        mov     %ax, %es
        mov     %ax, %fs
        mov     %ax, %gs

        /* set ss to kernel data segment in GDT, cs to kernel code segment in
         * GDT */
        /* stack layout according to x86-64-system figure 6-8 */
        shl     $3, %rdi        /* move data index to segment register format */
        mov     %rsp, %rax
        shl     $3, %rsi        /* move code index to segment register format */
        pop     %rcx            /* save return ip */
        push    %rdi            /* ss */
        push    %rax            /* rsp */
        pushfq                  /* rflags */
        push    %rsi            /* cs */
        push    %rcx            /* rip */
        iretq

/* this stub is jumped into by an individual interrupt vector (which also pushed
 * its interrupt number) */
.global interrupt_handler_stub
interrupt_handler_stub:
        pushaq
        mov     %rsp, %rdi      /* struct interrupt_frame has been constructed
                                 * on the stack */
        call    interrupt_handler
        popaq
        add     $16, %rsp       /* pop int number & error code/zero */
        iretq   /* return address & stack have been prepared by cpu */
