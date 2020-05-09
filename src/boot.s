.section .text
/* uefi enters _start in long mode with some kind of paging enabled (?). we just
 * hang here. */
.global _start
.type _start, @function
_start:
1:  jmp 1b

.size _start, . - _start
