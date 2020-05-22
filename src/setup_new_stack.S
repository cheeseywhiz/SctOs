#include "opsys/virtual-memory.h"

.section .text
/* void setup_new_stack(void *func, void *new_stack_bottom); */
.global setup_new_stack
.type setup_new_stack, @function
setup_new_stack:
  lea PAGE_SIZE(%rsi), %rsp
  xor %rbp, %rbp
  push %rbp
  jmp *%rdi
