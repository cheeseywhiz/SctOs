#include <asm/unistd.h>

.section .data
.global hello
.type hello, @object
hello:
    .string "hello\n"

.section .bss
.global space
.type space, @object
space:
    .skip 27

.section .text
.global _start
.type _start, @function
_start:
    lea hello(%rip), %rax
    mov (%eax), %dil
    mov $__NR_exit, %rax
    syscall
