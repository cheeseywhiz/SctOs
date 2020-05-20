#include "opsys/virtual-memory.h"

.section .text
/* void setup_new_stack(page_table_t *pt, void *func, UINT64 new_stack_bottom);
 * load the new stack and call func(pt) */
.global setup_new_stack
.type setup_new_stack, @function
setup_new_stack:
    lea PAGE_SIZE(%rdx), %rsp
    xor %rbp, %rbp
    push %rbp
    jmp *%rsi
