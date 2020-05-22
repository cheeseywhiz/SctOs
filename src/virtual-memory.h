#pragma once
#include "util.h"
#include "opsys/virtual-memory.h"

void free_physical_page(void *page);

enum app_flags {
    APP_NORMAL = 0,    /* return an uninitialized new page */
    APP_ZERO = 1 << 0, /* zero initialize the new page */
    APP_FLAT = 1 << 1, /* return the actual physical address of the new page. */
    APP_PTE  = 1 << 2, /* implies APP_ZERO and APP_FLAT */
};

__malloc void* allocate_physical_page(enum app_flags);

/* create a new address space according to virtual-memory.md */
page_table_t* new_address_space(void);
