/* this file provides tools for working with virtual memory */
#pragma once
#include <stdint.h>
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

/* these are set during load.
 * they would go in .data.ro_after_init like linux if I get around to
 * implementing that. */
extern uint64_t ram_size;
extern uint64_t paddr_max; /* maximum accessible physical page + 1 */
extern uint64_t paddr_base; /* -1GB - paddr_max */

typedef uint64_t page_table_entry_t[512];

/* x86-64-system table 4-19 */
enum page_table_entry_flags {
    PTE_P  = 1 << 0, /* present */
    PTE_RW = 1 << 1, /* true: read+write; false: read only */
    PTE_US = 1 << 2, /* true: supervisor; false: user */
    PTE_WT = 1 << 3, /* write through */
    PTE_CD = 1 << 4, /* cache disable */
    PTE_A  = 1 << 5, /* accessed */
    PTE_D  = 1 << 6, /* dirty */
    PTE_AT = 1 << 7, /* attribute table */
    PTE_G  = 1 << 8, /* global */
/* physical address to next paging level */
#define PTE_ADDR_MASK 0xfffffffff000
};
