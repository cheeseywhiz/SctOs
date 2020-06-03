/* this module provides functions for working with virtual memory */
#include <stddef.h>
#include "string.h"
#include "elf.h"
#include "opsys/virtual-memory.h"
#include "opsys/x86.h"
#include "opsys/bootloader_data.h"
#include "virtual-memory.h"
#include "x86.h"

struct free_page {
    struct free_page *next;
};

static struct free_page *free_list = NULL;

void free_physical_page(void *page)
{
    struct free_page *free_page = page;
    free_page->next = free_list;
    free_list = free_page;
}

void* allocate_physical_page(enum app_flags flags)
{
    if (flags & APP_PTE)
        flags |= APP_ZERO | APP_FLAT;
    if (!free_list)
        halt(); /* not implemented */
    struct free_page *page = free_list;
    free_list = free_list->next;
    if (flags & APP_ZERO)
        memset(page, 0, PAGE_SIZE);
    if (flags & APP_FLAT)
        return (void*)((uint64_t)page - bootloader_data->paddr_base);
    return page;
}

static void map_range(page_table_t*, uint64_t, uint64_t, uint64_t, uint64_t);
static void set_vpage_ro(page_table_t*, uint64_t);

/* create a new address space according to virtual-memory.md */
page_table_t* new_address_space(void)
{
    page_table_t *address_space;
    if (!(address_space = allocate_physical_page(APP_ZERO)))
        halt(); /* nomem */

    map_range(address_space,
        (uint64_t)bootloader_data - bootloader_data->paddr_base,
        (uint64_t)bootloader_data,
        1,
        PTE_RW);
    map_range(address_space,
        (uint64_t)bootloader_data->free_memory - bootloader_data->paddr_base,
        (uint64_t)bootloader_data->free_memory,
        bootloader_data->n_pages,
        PTE_RW);
    map_range(address_space, cpu.apic.paddr, cpu.apic.vaddr, 1, PTE_RW);

    /* runtime segments */
    for (UINT64 i = 0; i < bootloader_data->NumEntries; ++i) {
        EFI_MEMORY_DESCRIPTOR *Memory = &bootloader_data->MemoryMap[i];
        if (!(Memory->Attribute & EFI_MEMORY_RUNTIME))
            continue;
        uint64_t flags = 0;
        if (Memory->Type == EfiRuntimeServicesData
                || Memory->Type == EfiMemoryMappedIO
                || Memory->Type == EfiMemoryMappedIOPortSpace)
            flags |= PTE_RW;
        /* XXX: EfiRuntimeServicesCode needs RW?? this doesn't seem right but by
         * trial and error, runtime services code also contains r/w code
         * (specifically, the ResetSystem function) */
        if (Memory->Type == EfiRuntimeServicesCode)
            flags |= PTE_RW;
        map_range(address_space, Memory->PhysicalStart, Memory->VirtualStart,
                  Memory->NumberOfPages, flags);
    }

    /* kernel segments */
    const Elf64_Phdr *relro = NULL;

    for (Elf64_Half i = 0; i < bootloader_data->ehdr->e_phnum; ++i) {
        const Elf64_Phdr *phdr = &bootloader_data->phdrs[i];
        if (phdr->p_type == PT_GNU_RELRO)
            relro = phdr;
        if (phdr->p_type != PT_LOAD)
            continue;
        map_range(address_space,
            PAGE_BASE(phdr->p_paddr),
            PAGE_BASE(phdr->p_vaddr),
            NUM_PAGES(phdr->p_vaddr, phdr->p_memsz),
            phdr->p_flags & PF_W ? PTE_RW : 0);
    }

    if (relro) {
        for (Elf64_Xword i = 0;
                i < NUM_PAGES(relro->p_vaddr, relro->p_memsz);
                ++i)
            set_vpage_ro(address_space,
                PAGE_BASE(relro->p_vaddr) + PAGE_SIZE * i);
    }

    return address_space;
}

static void map_page(page_table_t*, uint64_t, uint64_t, uint64_t);

/* map n pages at vaddr to n pages at paddr. */
static void map_range(page_table_t *address_space, uint64_t paddr_start,
                      uint64_t vaddr_start, uint64_t n_pages, uint64_t flags)
{
    for (uint64_t i = 0; i < n_pages; ++i)
        map_page(address_space,
            paddr_start + PAGE_SIZE * i,
            vaddr_start + PAGE_SIZE * i,
            flags);
}

#define NEXT_PAGE_LEVEL(entry) \
    (page_table_t*)(bootloader_data->paddr_base + ((*entry) & PTE_ADDR_MASK))

/* map vpage to ppage. ppage is an actual physical page. (flat?) */
static void map_page(page_table_t *address_space, uint64_t ppage,
                     uint64_t vpage, uint64_t flags)
{
    /* x86-64-system figure 4-8
     * opsys-loader.c map_page */
    pte_t *level_4_e = &(*address_space)[PAGE_LEVEL_INDEX(vpage, 4)];
    if (!(*level_4_e & PTE_P))
        *level_4_e = (uint64_t)allocate_physical_page(APP_PTE) | PTE_P | PTE_RW;

    page_table_t *level_3 = NEXT_PAGE_LEVEL(level_4_e);
    pte_t *level_3_e = &(*level_3)[PAGE_LEVEL_INDEX(vpage, 3)];
    if (!(*level_3_e & PTE_P))
        *level_3_e = (uint64_t)allocate_physical_page(APP_PTE) | PTE_P | PTE_RW;

    page_table_t *level_2 = NEXT_PAGE_LEVEL(level_3_e);
    pte_t *level_2_e = &(*level_2)[PAGE_LEVEL_INDEX(vpage, 2)];
    if (!(*level_2_e & PTE_P))
        *level_2_e = (uint64_t)allocate_physical_page(APP_PTE) | PTE_P | PTE_RW;

    page_table_t *level_1 = NEXT_PAGE_LEVEL(level_2_e);
    pte_t *level_1_e = &(*level_1)[PAGE_LEVEL_INDEX(vpage, 1)];
    if (*level_1_e & PTE_P)
        halt(); /* remap */
    *level_1_e = ppage | PTE_P | flags;
}

/* turn off the RW flag for the given virtual address */
void set_vpage_ro(page_table_t *address_space, uint64_t vpage)
{
    pte_t *level_4_e = &(*address_space)[PAGE_LEVEL_INDEX(vpage, 4)];
    if (!(*level_4_e & PTE_P))
        halt(); /* assert */

    page_table_t *level_3 = NEXT_PAGE_LEVEL(level_4_e);
    pte_t *level_3_e = &(*level_3)[PAGE_LEVEL_INDEX(vpage, 3)];
    if (!(*level_3_e & PTE_P))
        halt(); /* assert */

    page_table_t *level_2 = NEXT_PAGE_LEVEL(level_3_e);
    pte_t *level_2_e = &(*level_2)[PAGE_LEVEL_INDEX(vpage, 2)];
    if (!(*level_2_e & PTE_P))
        halt(); /* assert */

    page_table_t *level_1 = NEXT_PAGE_LEVEL(level_2_e);
    pte_t *level_1_e = &(*level_1)[PAGE_LEVEL_INDEX(vpage, 1)];
    if (!(*level_1_e & PTE_P))
        halt(); /* assert */
    *level_1_e &= ~((uint64_t)PTE_RW);
}
