/* this file loads the kernel executable into memory and runs it */
#include <efi.h>
#include <efilib.h>
#include "efi-wrapper.h"
#include "readelf.h"
#include "elf.h"
#include "util.h"
#include "virtual-memory.h"

static void print_program_headers(const Elf64_Ehdr*, const Elf64_Phdr*);
static void load_elf_pages(EFI_FILE_HANDLE, const Elf64_Ehdr*,
                            Elf64_Phdr*);
static void init_mmap(EFI_MEMORY_DESCRIPTOR*, UINT64, const Elf64_Ehdr*,
                           const Elf64_Phdr*);
static void print_memory_map(EFI_MEMORY_DESCRIPTOR*, UINT64);

EFI_STATUS
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS Status = EFI_SUCCESS;
    InitializeLib(ImageHandle, SystemTable);

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
    /* tianocore size is longer than spec by 8 bytes for some reason,
     * so we check that we're using a patched gnu-efi */
    EFI_ASSERT(DescriptorSize == sizeof(*MemoryMap));
    init_mmap(MemoryMap, NumEntries, ehdr, phdrs);
    print_memory_map(MemoryMap, NumEntries);

    return EFI_SUCCESS;
}

static void print_program_header(const Elf64_Phdr*);

static void
print_program_headers(const Elf64_Ehdr *ehdr, const Elf64_Phdr *phdrs)
{
    if (!ehdr->e_phnum)
        return;
    Print(L"program headers:\n");
    Print(L"%-8s %5s %8s %8s %8s %8s %8s %8s\n",
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

    Print(L"%c%c%c   %8lx %8lx %8lx %8lx %8lx %8lx\n",
        phdr->p_flags & PF_R ? L'R' : L' ',
        phdr->p_flags & PF_W ? L'W' : L' ',
        phdr->p_flags & PF_X ? L'X' : L' ',
        phdr->p_offset, phdr->p_filesz,
        phdr->p_paddr,
        phdr->p_vaddr, phdr->p_memsz,
        NUM_PAGES(phdr->p_vaddr, phdr->p_memsz));
}

/* allocate Loader(Code|Data) pages, read file into pages, then set p_paddr for
 * each phdr */
static void
load_elf_pages(EFI_FILE_HANDLE File, const Elf64_Ehdr *ehdr, Elf64_Phdr *phdrs)
{
    for (Elf64_Half i = 0; i < ehdr->e_phnum; ++i) {
        Elf64_Phdr *phdr = &phdrs[i];
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

/* write virtual addresses of segments into the efi page structure,
 * identity map alread-loaded Loder(Code|Data) segments */
static void
init_mmap(EFI_MEMORY_DESCRIPTOR *MemoryMap, UINT64 NumEntries,
          const Elf64_Ehdr *ehdr, const Elf64_Phdr *phdrs)
{
    for (UINT64 i = 0; i < NumEntries; ++i) {
        EFI_MEMORY_DESCRIPTOR *Memory = &MemoryMap[i];
        if (Memory->Type != EfiLoaderCode && Memory->Type != EfiLoaderData)
            continue;
        const Elf64_Phdr *segment = NULL;

        for (Elf64_Half j = 0; j < ehdr->e_phnum; ++j) {
            const Elf64_Phdr *tmp = &phdrs[j];
            if (tmp->p_type != PT_LOAD
                    || PAGE_BASE(tmp->p_paddr) != Memory->PhysicalStart)
                continue;
            segment = tmp;
            break;
        }

        if (segment) /* we are loading descriptor */
            Memory->VirtualStart = KERNEL_BASE + PAGE_BASE(segment->p_vaddr);
        else /* we're running on descriptor */
            Memory->VirtualStart = Memory->PhysicalStart;
    }
}

static void print_memory_descriptor(EFI_MEMORY_DESCRIPTOR*);

static void
print_memory_map(EFI_MEMORY_DESCRIPTOR *MemoryMap, UINT64 NumEntries)
{
    Print(L"MemoryMap: %lu entries\n", NumEntries);
    Print(L"%24s %8s %16s %8s %8s %s\n",
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
    Print(L"%24s %8lx %16lx %8lu ",
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
