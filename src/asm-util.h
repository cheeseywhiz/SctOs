; vim: ft=asm
/* this file provides asm quality of life improvements */

/* push r15-r8, rax, rcx, rdx, rbx, original rsp, rbp, rsi, and rdi */
.macro pushaq
        push    %r15
        lea     8(%rsp), %r15   /* save original rsp */
        push    %r14
        push    %r13
        push    %r12
        push    %r11
        push    %r10
        push    %r9
        push    %r8
        /* below here is directly analogous to pushad */
        push    %rax
        push    %rcx
        push    %rdx
        push    %rbx
        push    %r15
        push    %rbp
        push    %rsi
        push    %rdi
.endm

/* pop rdi, rsi, rbp, original rsp, rbx, rdx, rcx, rax, r8-15 */
.macro popaq
        pop     %rdi
        pop     %rsi
        pop     %rbp
        add     $8, %rsp        /* skip */
        pop     %rbx
        pop     %rdx
        pop     %rcx
        pop     %rax
        /* above here is directly analogous to popad */
        pop     %r8
        pop     %r9
        pop     %r10
        pop     %r11
        pop     %r12
        pop     %r13
        pop     %r14
        pop     %r15
.endm
