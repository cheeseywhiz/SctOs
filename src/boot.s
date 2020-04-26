.set ALIGN,     1 << 0
.set MEMINFO,   1 << 1
.set FLAGS,     ALIGN | MEMINFO
.set MAGIC,     0x1BADB002
.set CHECKSUM,  -(MAGIC + FLAGS)

/* multiboot header */
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

.section .bootstrap_stack, "aw", @nobits
stack_bottom:
.skip 16384 /* 16 KiB */
stack_top:

.section .bss, "aw", @nobits
    .align 4096
boot_page_directory:
    .skip 4096
boot_page_table1:
    .skip 4096

.section .text
.global _start
.type _start, @function
_start:
    /* TODO: start at the first kernel page instead
     * TODO: identity map the first 1MB */
    movl $(boot_page_table1 - 0xC0000000), %edi
    movl $0, %esi
    movl $1023, %ecx

1:
    /* if (si < _kernel_start - 0xC0000000) continue; */
    cmpl $(_kernel_start - 0xC0000000), %esi
    jl 2f
    /* if (si >= _kernel_end - 0xC0000000) break; */
    cmpl $(_kernel_end - 0xC0000000), %esi
    jge 3f

    /* TODO: don't set .text and .rodata as read/write */

    /* add the current kernel page to the table with read/write permission */
    movl %esi, %edx
    orl $0x3, %edx
    movl %edx, (%edi)

2:
    addl $4096, %esi
    addl $4, %edi
    loop 1b

3:
    /* map 0xC03FF000 to vga memory */
    movl $(0xB8000 | 0x3), boot_page_table1 - 0xC0000000 + 1023 * 4

    /* map the new page table to 0 directory and 0xC00 directory */
    movl $(boot_page_table1 - 0xC0000000 + 0x3), boot_page_directory - 0xC0000000 + 0
    movl $(boot_page_table1 - 0xC0000000 + 0x3), boot_page_directory - 0xC0000000 + 768 * 4

    /* cr3 points to page directory */
    movl $(boot_page_directory - 0xC0000000), %ecx
    movl %ecx, %cr3

    /* enable paging and write protect bits */
    movl %cr0, %ecx
    orl $0x80010000, %ecx
    movl %ecx, %cr0

    /* jump to higher half */
    lea 1f, %ecx
    jmp *%ecx

1:
    movl $0, boot_page_directory + 0
    /* force TLB flush to commit changes */
    movl %cr3, %ecx
    movl %ecx, %cr3

    mov $stack_top, %esp
    call kernel_main
    cli
1:  hlt
    jmp 1b

.size _start, . - _start
