/* this file provides tools for working with virtual memory */
#pragma once
#ifndef __ASSEMBLER__
#include <stdint.h>
#endif

#ifdef __ASSEMBLER__
#define PAGE_SIZE 0x1000
#else
#define PAGE_SIZE 0x1000ULL
/* the base address of the page containing addr */
#define PAGE_BASE(addr) ((addr) & ~(PAGE_SIZE - 1))
/* the base address of the next page */
#define NEXT_PAGE(addr) PAGE_BASE((addr) + PAGE_SIZE)
/* the offset of the given address within its own page */
#define PAGE_OFFSET(addr) ((addr) & (PAGE_SIZE - 1))
/* the number of pages spanned by addresses in [base, base + size).
 * base need not be page aligned. */
#define NUM_PAGES(base, size) \
    (NEXT_PAGE((base) + (size) - 1 - PAGE_BASE(base)) / PAGE_SIZE)

/* numerical constants of byte amounts. p/n is sign +/-. */
#define p1KB 1024ULL
#define p1MB (p1KB * p1KB)
#define p1GB (p1KB * p1MB)
#define n1GB (-p1GB)

/* mem_layout.md */
#define KERNEL_BASE n1GB
/* see also bootloader_data for other layout variables */

/* x86-64-system figure 4-8: the index within the given (1-indexed) page level
 * that the vaddr refers to, e.g. 4 for the index in PML4, 1 for the index in
 * the page table. */
#define PAGE_LEVEL_INDEX(vaddr, page_level) \
    (((vaddr) >> (12 + 9 * ((page_level) - 1))) & 0x1ff)

typedef uint64_t pte_t;
typedef pte_t page_table_t[512];

/* x86-64-system table 4-19 */
enum page_table_entry_flags {
    PTE_P  = 1 << 0, /* present */
    PTE_RW = 1 << 1, /* true: read+write; false: read only */
    PTE_US = 1 << 2, /* true: user; false: supervisor */
    PTE_WT = 1 << 3, /* write through */
    PTE_CD = 1 << 4, /* cache disable */
    PTE_A  = 1 << 5, /* accessed */
    PTE_D  = 1 << 6, /* dirty */
    PTE_AT = 1 << 7, /* level 1: reference high PAT entries 4-7 */
    PTE_PS = 1 << 7, /* level 3, 2: page size. keep 0 for 4kb pages. */
                     /* level 4: reserved */
    PTE_G  = 1 << 8, /* global */
#define PTE_ADDR_MASK 0xfffffffff000 /* physical address to next paging level */
#define PTE_XD (1ULL << 63) /* execute disable */
};

#endif /* __ASSEMBLER */
