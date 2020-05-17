/* this file loads the kernel executable into memory and runs it */
#include <efi.h>
#include <efilib.h>
#include "efi-wrapper.h"
#include "readelf.h"
#include "elf.h"
#include "util.h"
#include "virtual-memory.h"

static void print_program_headers(const Elf64_Ehdr*, const Elf64_Phdr*);
static void load_elf_pages(EFI_FILE_HANDLE, const Elf64_Ehdr*, Elf64_Phdr*);
static void init_mmap(EFI_MEMORY_DESCRIPTOR*, UINT64*, UINT64*, UINT64*);
static void print_memory_map(EFI_MEMORY_DESCRIPTOR*, UINT64);

EFI_STATUS
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS Status = EFI_SUCCESS;
    InitializeLib(ImageHandle, SystemTable);

    /* TODO: bootloader sequence
     * acquire preliminary memory map
     * load kernel executable into physical memory
     * prepare boot page tables with Loader segments identity mapped
     * map kernel to high half
     * acquire final memory map
     * ExitBootServices
     * SetVirtualAddressMap
     * enable paging with boot page tables
     * finish preparation of kernel executable
     * jump to kernel
     */

    EFI_LOADED_IMAGE *LoadedImage;
    if (_EFI_ERROR(Status = uefi_call_wrapper(BS->HandleProtocol, 3,
            ImageHandle, &LoadedImageProtocol, (void**)&LoadedImage)))
        EXIT_STATUS(Status, L"handle LoadedImageProtocol");
    EFI_FILE_HANDLE RootDir;
    if (!(RootDir = LibOpenRoot(LoadedImage->DeviceHandle)))
        EXIT_STATUS(EFI_ABORTED, L"LibOpenRoot");
    EFI_FILE_HANDLE File;
    if (_EFI_ERROR(Status = uefi_call_wrapper(RootDir->Open, 5,
            RootDir, &File, L"\\opsys", EFI_FILE_MODE_READ, 0)))
        EXIT_STATUS(Status, L"RootDir->Open");

    Elf64_Ehdr _ehdr, *ehdr = &_ehdr;
    Elf64_Phdr *phdrs = NULL;
    read_program_headers(File, ehdr, &phdrs);
    load_elf_pages(File, ehdr, phdrs);
    uefi_call_wrapper(File->Close, 1, File);
    uefi_call_wrapper(RootDir->Close, 1, RootDir);
    print_program_headers(ehdr, phdrs);

    UINT64 NumEntries, MapKey, DescriptorSize;
    UINT32 DescriptorVersion;
    EFI_MEMORY_DESCRIPTOR *MemoryMap;
    if (!(MemoryMap = LibMemoryMap(&NumEntries, &MapKey, &DescriptorSize,
                                   &DescriptorVersion)))
        EXIT_STATUS(EFI_ABORTED, L"LibMemoryMap");
    /* edk2 size is longer than spec by 8 bytes for some reason,
     * so check that we're using a patched gnu-efi */
    EFI_ASSERT(DescriptorSize == sizeof(*MemoryMap));
    print_memory_map(MemoryMap, NumEntries);
    Print(L"ExitBootServices: 0x%lx\n", BS->ExitBootServices);
    UINT64 RamSize, PaddrMax;
    init_mmap(MemoryMap, &NumEntries, &RamSize, &PaddrMax);
    print_memory_map(MemoryMap, NumEntries);
    Print(L"RamSize:    0x%lx\n", RamSize);
    Print(L"PaddrMax:   0x%lx\n", PaddrMax);
    Print(L"MemoryMap:  0x%lx\n", MemoryMap);

    return EFI_SUCCESS;
}

static void print_program_header(const Elf64_Phdr*);

static void
print_program_headers(const Elf64_Ehdr *ehdr, const Elf64_Phdr *phdrs)
{
    if (!ehdr->e_phnum)
        return;
    Print(L"program headers:\n");
    Print(L"%-8s %5s %8s %8s %8s %16s %8s %8s\n",
        L"type", L"flags", L"offset", L"filesz", L"paddr", L"vaddr", L"memsz",
        L"n pages");
    for (Elf64_Half i = 0; i < ehdr->e_phnum; ++i)
        print_program_header(&phdrs[i]);
}

static void
print_program_header(const Elf64_Phdr *phdr)
{
    if (phdr->p_type < PT_NUM)
        Print(L"%-8s ", a2u(elf_segment_type_str[phdr->p_type]));
    else if (PT_GNU_STACK <= phdr->p_type && phdr->p_type <= PT_GNU_RELRO)
        Print(L"%-8s ",
            a2u(elf_segment_type_str[PT_NUM + phdr->p_type - PT_GNU_STACK]));
    else
        Print(L"%-8x ", phdr->p_type);

    Print(L"%c%c%c   %8lx %8lx %8lx %16lx %8lx %8lx\n",
        phdr->p_flags & PF_R ? L'R' : L' ',
        phdr->p_flags & PF_W ? L'W' : L' ',
        phdr->p_flags & PF_X ? L'X' : L' ',
        phdr->p_offset, phdr->p_filesz,
        phdr->p_paddr,
        phdr->p_vaddr, phdr->p_memsz,
        NUM_PAGES(phdr->p_vaddr, phdr->p_memsz));
}

/* allocate Loader(Code|Data) pages, read file into pages, then set p_paddr and
 * p_vaddr for each phdr */
static void
load_elf_pages(EFI_FILE_HANDLE File, const Elf64_Ehdr *ehdr, Elf64_Phdr *phdrs)
{
    for (Elf64_Half i = 0; i < ehdr->e_phnum; ++i) {
        Elf64_Phdr *phdr = &phdrs[i];
        if (phdr->p_type != PT_GNU_STACK)
            phdr->p_vaddr += KERNEL_BASE;
        if (phdr->p_type != PT_LOAD)
            continue;
        EFI_STATUS Status;
        if (_EFI_ERROR(Status = uefi_call_wrapper(BS->AllocatePages, 4,
                AllocateAnyPages,
                phdr->p_flags & PF_X ? EfiLoaderCode : EfiLoaderData,
                NUM_PAGES(phdr->p_vaddr, phdr->p_memsz), &phdr->p_paddr)))
            EXIT_STATUS(Status, L"AllocatePages");
        /* data segment is not page aligned */
        phdr->p_paddr += PAGE_OFFSET(phdr->p_vaddr);
        elf_read(File, (void*)phdr->p_paddr, phdr->p_offset, phdr->p_filesz);
    }

    for (Elf64_Half i = 0; i < ehdr->e_phnum; ++i) {
        Elf64_Phdr *phdr = &phdrs[i];
        if (phdr->p_type == PT_LOAD)
            continue;
        Elf64_Phdr *segment = NULL;

        for (Elf64_Half j = 0; j < ehdr->e_phnum; ++j) {
            Elf64_Phdr *tmp = &phdrs[j];
            if (tmp->p_type != PT_LOAD
                    || !IN_RANGE(tmp->p_vaddr, tmp->p_memsz, phdr->p_vaddr)
                    || !IN_RANGE(tmp->p_vaddr, tmp->p_memsz,
                             phdr->p_vaddr + phdr->p_memsz - 1))
                continue;
            segment = tmp;
            break;
        }

        phdr->p_paddr = segment
            ? PAGE_OFFSET(phdr->p_vaddr) + PAGE_BASE(segment->p_paddr)
            : 0;
    }
}

/* parse and complete filling out the memory map */
static void
init_mmap(EFI_MEMORY_DESCRIPTOR *MemoryMap, UINT64 *NumEntries, UINT64 *RamSize,
    UINT64 *PaddrMax)
{
    /* determine RamSize, PaddrMax, MmioSize */
    *RamSize = 0;
    *PaddrMax = 0;
    UINT64 MmioSize = 0;

    for (UINT64 i = 0; i < *NumEntries; ++i) {
        EFI_MEMORY_DESCRIPTOR *Memory = &MemoryMap[i];
        if (Memory->Type == EfiMemoryMappedIO
                || Memory->Type == EfiMemoryMappedIOPortSpace)
            MmioSize += Memory->NumberOfPages * PAGE_SIZE;
        if (i != *NumEntries - 1 - 2)
            continue;
        EFI_MEMORY_DESCRIPTOR *Next = &MemoryMap[i + 1];
        *PaddrMax = *RamSize =
            Next->PhysicalStart + Next->NumberOfPages * PAGE_SIZE;
        *RamSize = Next->Type == EfiConventionalMemory
            ? Memory->PhysicalStart + Memory->NumberOfPages * PAGE_SIZE
              + Next->NumberOfPages * PAGE_SIZE
            : *PaddrMax;
    }

    /* merge consecutive entries. also, reclaim certain efi segments. */
    EFI_ASSERT(*NumEntries != 0);
    EFI_MEMORY_DESCRIPTOR *Prev = &MemoryMap[0];
    Prev->Type = EfiConventionalMemory;
    UINT64 UsableSize = PAGE_SIZE;
    UINT64 NewLength = 1;

    for (UINT64 i = 1; i < *NumEntries; ++i) {
        EFI_MEMORY_DESCRIPTOR *Memory = &MemoryMap[i];
        /* uefi table 30: reclaim segment if usable by os */
        if (TRUE
            && Memory->Type != EfiReservedMemoryType
            && Memory->Type != EfiLoaderCode
            && Memory->Type != EfiLoaderData
            && Memory->Type != EfiRuntimeServicesCode
            && Memory->Type != EfiRuntimeServicesData
            && Memory->Type != EfiUnusableMemory
            && Memory->Type != EfiMemoryMappedIO
            && Memory->Type != EfiMemoryMappedIOPortSpace
        )
            Memory->Type = EfiConventionalMemory;

        if (Memory->Type == EfiConventionalMemory)
            UsableSize += Memory->NumberOfPages * PAGE_SIZE;

        /* can be merged? */
        if (Memory->Type == Prev->Type
            && Memory->Attribute == Prev->Attribute
            /* physical pages are consecutive? */
            && Memory->PhysicalStart == Prev->PhysicalStart
                                        + Prev->NumberOfPages * PAGE_SIZE
        ) {
            Prev->NumberOfPages += Memory->NumberOfPages;
        } else {
            Prev = &MemoryMap[NewLength++];
            *Prev = *Memory;
        }
    }

    Print(L"UsableSize: 0x%lx\n", UsableSize);
    *NumEntries = NewLength;

    /* request virtual mapping for runtime segments */
    UINT64 PaddrBase = KERNEL_BASE - *PaddrMax;
    UINT64 MmioLoadVaddr = PaddrBase - MmioSize;

    for (UINT64 i = 0; i < *NumEntries; ++i) {
        EFI_MEMORY_DESCRIPTOR *Memory = &MemoryMap[i];

        if (Memory->Type == EfiMemoryMappedIO
                || Memory->Type == EfiMemoryMappedIOPortSpace) {
            Memory->VirtualStart = MmioLoadVaddr;
            MmioLoadVaddr += Memory->NumberOfPages * PAGE_SIZE;
        } else if (Memory->Attribute & EFI_MEMORY_RUNTIME) {
            Memory->VirtualStart = Memory->PhysicalStart + PaddrBase;
        }
    }
}

static void print_memory_descriptor(EFI_MEMORY_DESCRIPTOR*);

static void
print_memory_map(EFI_MEMORY_DESCRIPTOR *MemoryMap, UINT64 NumEntries)
{
    Print(L"MemoryMap: %lu entries\n", NumEntries);
    Print(L"%24s %9s %16s %8s %s\n",
        L"type", L"paddr", L"vaddr", L"num", L"flags");
    for (UINT64 i = 0; i < NumEntries; ++i)
        print_memory_descriptor(&MemoryMap[i]);
}

#define PRINT_ATTR(suffix) \
    if (Memory->Attribute & (EFI_MEMORY_ ## suffix)) \
        Print(L"%-3s ", (L ## #suffix))

static void
print_memory_descriptor(EFI_MEMORY_DESCRIPTOR *Memory)
{
    Print(L"%24s %9lx %16lx %8lu ",
        efi_memory_type_str[Memory->Type],
        Memory->PhysicalStart,
        Memory->VirtualStart, Memory->NumberOfPages);
    PRINT_ATTR(XP);
    PRINT_ATTR(RP);
    PRINT_ATTR(WP);
    PRINT_ATTR(UCE);
    PRINT_ATTR(WB);
    PRINT_ATTR(WT);
    PRINT_ATTR(WC);
    PRINT_ATTR(UC);
    PRINT_ATTR(RUNTIME);
    Print(L"\n");
}
