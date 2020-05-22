#pragma once
#include <stdint.h>
#include <efi.h>
#include "elf.h"

struct bootloader_data {
    void *free_memory;
    uint64_t n_pages;
    uint64_t ram_size;
    uint64_t paddr_base, paddr_size;
    uint64_t mmio_base, mmio_size;
    const Elf64_Ehdr *ehdr;
    const Elf64_Phdr *phdrs;
    EFI_RUNTIME_SERVICES *RT;
    UINT64 NumEntries;
    EFI_MEMORY_DESCRIPTOR MemoryMap[];
};

#ifdef _KERNEL
/* set during kernel boot */
extern struct bootloader_data *bootloader_data;
#endif
