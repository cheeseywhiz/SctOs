/* this file provides tools for working with virtual memory */
#pragma once
#define PAGE_SIZE 0x1000ULL
/* the base address of the page containing addr */
#define PAGE_BASE(addr) ((addr) & ~(PAGE_SIZE - 1))
/* the base address of the next page */
#define NEXT_PAGE(addr) PAGE_BASE((addr) + PAGE_SIZE)
/* the offset of the given address within its own page */
#define PAGE_OFFSET(addr) ((addr) & (PAGE_SIZE - 1))
/* the number of pages spanned by addresses in [base, base + size) */
#define NUM_PAGES(base, size) \
    (NEXT_PAGE((base) + (size) - 1 - PAGE_BASE(base)) / PAGE_SIZE)

/* this is the "high half", where kernel memory lives, at -1GB */
#define KERNEL_BASE 0xffffffffc0000000

