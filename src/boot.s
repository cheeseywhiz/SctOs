.section .text
/* uefi enters _start in long mode with the kernel mapped to the higher half.
 * we generate an exception so they we can inspect the state of the computer,
 * then we loop just in case. */
.global _start
.type _start, @function
_start:
    mov $0, %rax
    div %rax
1:  jmp 1b

.size _start, . - _start
