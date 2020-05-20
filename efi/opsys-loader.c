/* this file loads and executes the kernel executable */
#include <efi.h>
#include <efilib.h>
#include <stdint.h>
#include "efi-wrapper.h"
#include "readelf.h"
#include "elf.h"
#include "util.h"
#include "x86.h"
#include "opsys/virtual-memory.h"

static const CHAR16 *const kernel_fname = L"\\opsys";
static EFI_FILE_HANDLE RootDir = NULL;

static void efi_debug_entry(EFI_LOADED_IMAGE*);
static void print_cr0(void);
static void print_cr4(void);
static void print_efer(void);

/* gdb.py sets this function as a breakpoint. then, set a command to run
 * "finish" upon reaching this function. */
static void
efi_debug_entry(EFI_LOADED_IMAGE *LoadedImage)
{
    Print(L"ImageBase: 0x%lx\n", LoadedImage->ImageBase);
    print_cr0();
    page_table_t *cr3 = get_cr3();
    Print(L"cr3: 0x%lx\n", cr3);
    print_cr4();
    print_efer();
}

void* memset(void*, int, size_t);
static void load_kernel(const Elf64_Ehdr**, const Elf64_Phdr**);
static void print_program_headers(const Elf64_Ehdr*, const Elf64_Phdr*);
static page_table_t*  prepare_boot_page_tables(
    EFI_MEMORY_DESCRIPTOR*, UINT64, const Elf64_Ehdr*, const Elf64_Phdr*);
static UINT64 allocate_page(void);
typedef void efi_main2_t(page_table_t*);
static __noreturn efi_main2_t efi_main2;
extern void setup_new_stack(page_table_t*, efi_main2_t, UINT64);
static void init_mmap(EFI_MEMORY_DESCRIPTOR*, UINT64*, UINT64*, UINT64*);
static void print_memory_map(EFI_MEMORY_DESCRIPTOR*, UINT64);

/* UEFI firmware calls efi_main in long mode with 4-level paging enabled (with
 * write protect) with an identity mapping */
EFI_STATUS
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS Status = EFI_SUCCESS;
    InitializeLib(ImageHandle, SystemTable);
    EFI_LOADED_IMAGE *LoadedImage;
    if (_EFI_ERROR(Status = uefi_call_wrapper(BS->HandleProtocol, 3,
            ImageHandle, &LoadedImageProtocol, (void**)&LoadedImage)))
        EXIT_STATUS(Status, L"handle LoadedImageProtocol");
    efi_debug_entry(LoadedImage);
    if (!(RootDir = LibOpenRoot(LoadedImage->DeviceHandle)))
        EXIT_STATUS(EFI_ABORTED, L"LibOpenRoot");

    /* 
     * bootloader sequence
     * 1. allocate new stack
     * 2. acquire preliminary memory map
     * 3. load kernel executable into physical memory
     * 4. prepare boot page tables with Loader segments identity mapped and
     *    kernel mapped to high half
     * 5. acquire final memory map
     * 6. ExitBootServices
     * 7. SetVirtualAddressMap
     * 8. set up new stack
     * 9. enable paging with boot page tables
     * 10. finish preparation of kernel executable
     * 11. jump to kernel
     */

    /* 1. allocate new stack */
    UINT64 new_stack = allocate_page();

    /* 2. acquire preliminary memory map */
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

    /* 3. load kernel executable into physical memory */
    const Elf64_Ehdr *ehdr;
    const Elf64_Phdr *phdrs;
    load_kernel(&ehdr, &phdrs);
    print_program_headers(ehdr, phdrs);
    uefi_call_wrapper(RootDir->Close, 1, RootDir);
    RootDir = NULL;

    /* 4. prepare boot page tables */
    page_table_t *boot_page_table =
        prepare_boot_page_tables(MemoryMap, NumEntries, ehdr, phdrs);
    FreePool(MemoryMap);

    /* 5. acquire final memory map */
    if (!(MemoryMap = LibMemoryMap(&NumEntries, &MapKey, &DescriptorSize,
                                   &DescriptorVersion)))
        EXIT_STATUS(EFI_ABORTED, L"LibMemoryMap");

    /* 6. ExitBootServices */
    if (_EFI_ERROR(Status = uefi_call_wrapper(BS->ExitBootServices, 2,
            ImageHandle, MapKey)))
        EXIT_STATUS(Status, L"ExitBootServices");

    /* 7. SetVirtualAddressMap */
    UINT64 RamSize, PaddrMax;
    init_mmap(MemoryMap, &NumEntries, &RamSize, &PaddrMax);
    if (_EFI_ERROR(Status = uefi_call_wrapper(RT->SetVirtualAddressMap, 4,
            NumEntries * sizeof(*MemoryMap), DescriptorSize, DescriptorVersion,
            MemoryMap)))
        halt();

    /* 8. set up new stack */
    setup_new_stack(boot_page_table, efi_main2, new_stack);
    /* control transfers almost directly to efi_main2 with new stack */
    __builtin_unreachable();
}

static void
efi_main2(page_table_t *boot_page_table)
{
    /* 9. enable paging with boot page tables */
    set_cr3(boot_page_table);

    /* 10. finish preparation of kernel executable */
    /* XXX:
     * need to implement these as they become needed
     * - relocations,
     * - set load-time synbols
     * - clear bss
     * - set relro pages to ro
     */
    const Elf64_Ehdr *ehdr = (void*)KERNEL_BASE;

    /* 11. jump to kernel */
    void (*main)(void) = (void (*)(void))ehdr->e_entry;
    main();
    __builtin_unreachable();
}

#define PRINT_CR0(flag) \
    if (cr0 & CR0_ ## flag) \
        Print(L"%s ", (L ## #flag))

static void
print_cr0(void)
{
    uint64_t cr0 = get_cr0();
    Print(L"cr0: ");
    PRINT_CR0(PE);
    PRINT_CR0(MP);
    PRINT_CR0(EM);
    PRINT_CR0(TS);
    PRINT_CR0(ET);
    PRINT_CR0(NE);
    PRINT_CR0(WP);
    PRINT_CR0(AM);
    PRINT_CR0(NW);
    PRINT_CR0(CD);
    PRINT_CR0(PG);
    Print(L"\n");
}

#define PRINT_CR4(flag) \
    if (cr4 & CR4_ ## flag) \
        Print(L"%s ", (L ## #flag))

static void
print_cr4(void)
{
    uint64_t cr4 = get_cr4();
    Print(L"cr4: ");
    PRINT_CR4(VME);
    PRINT_CR4(PVI);
    PRINT_CR4(TSD);
    PRINT_CR4(DE);
    PRINT_CR4(PSE);
    PRINT_CR4(PAE);
    PRINT_CR4(MCE);
    PRINT_CR4(PGE);
    PRINT_CR4(PCE);
    PRINT_CR4(OSFXSR);
    PRINT_CR4(OSXMMEXCPT);
    PRINT_CR4(UMIP);
    PRINT_CR4(VMXE);
    PRINT_CR4(SMXE);
    PRINT_CR4(FSGSBASE);
    PRINT_CR4(PCIDE);
    PRINT_CR4(OSXSAVE);
    PRINT_CR4(SMEP);
    PRINT_CR4(SMAP);
    PRINT_CR4(PKE);
    Print(L"\n");
}

#define PRINT_EFER(flag) \
    if (efer & EFER_ ## flag) \
        Print(L"%s ", (L ## #flag))

static void
print_efer(void)
{
    uint64_t efer = read_msr(IA32_EFER);
    Print(L"ia32_efer: ");
    PRINT_EFER(SYSCALL);
    PRINT_EFER(LME);
    PRINT_EFER(LMA);
    PRINT_EFER(NXE);
    Print(L"\n");
}

static void load_elf_pages(EFI_FILE_HANDLE, const Elf64_Ehdr*, Elf64_Phdr*);

/* copy the kernel into memory. ehdr_out and phdrs_out will point to the
 * fixed-up structures in the loaded image. */
static void
load_kernel(const Elf64_Ehdr **ehdr_out, const Elf64_Phdr **phdrs_out)
{
    EFI_ASSERT(RootDir);
    EFI_STATUS Status;
    EFI_FILE_HANDLE File;
    if (_EFI_ERROR(Status = uefi_call_wrapper(RootDir->Open, 5,
            RootDir, &File, (CHAR16*)kernel_fname, EFI_FILE_MODE_READ, 0)))
        EXIT_STATUS(Status, L"RootDir->Open");
    Elf64_Ehdr _ehdr, *ehdr = &_ehdr;
    Elf64_Phdr *phdrs;
    read_program_headers(File, ehdr, &phdrs);
    load_elf_pages(File, ehdr, phdrs);
    uefi_call_wrapper(File->Close, 1, File);

    /* fix up the corresponding headers in the loaded image */
    Elf64_Phdr *first_phdr = &phdrs[0];
    /* XXX: if this constraint is ever broken we'll just have to search for the
     * phdr with p_offset 0. */
    EFI_ASSERT(first_phdr->p_type == PT_LOAD && first_phdr->p_offset == 0);
    *ehdr_out = (Elf64_Ehdr*)first_phdr->p_paddr;
    *phdrs_out = (Elf64_Phdr*)(first_phdr->p_paddr + (*ehdr_out)->e_phoff);
    (*(Elf64_Ehdr**)ehdr_out)->e_entry += KERNEL_BASE;

    for (Elf64_Half i = 0; i < ehdr->e_phnum; ++i) {
        Elf64_Phdr *phdr = &phdrs[i];
        Elf64_Phdr *phdr_out = (Elf64_Phdr*)(&(*phdrs_out)[i]);
        phdr_out->p_paddr = phdr->p_paddr;
        phdr_out->p_vaddr = phdr->p_vaddr;
    }

    elf_free(phdrs);
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

    /* set p_paddr for non-LOAD segments (e.g. DYNAMIC) */
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

static void print_program_header(const Elf64_Phdr*);

static void
print_program_headers(const Elf64_Ehdr *ehdr, const Elf64_Phdr *phdrs)
{
    if (!ehdr->e_phnum)
        return;
    Print(L"program headers:\n");
    Print(L"%-8s %5s %8s %8s %8s %16s %8s %s\n",
        L"type", L"flags", L"offset", L"filesz", L"vaddr", L"memsz", L"paddr",
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

    Print(L"%c%c%c   %8lx %8lx %16lx %8lx %8lx %lu\n",
        phdr->p_flags & PF_R ? L'R' : L' ',
        phdr->p_flags & PF_W ? L'W' : L' ',
        phdr->p_flags & PF_X ? L'X' : L' ',
        phdr->p_offset, phdr->p_filesz,
        phdr->p_vaddr, phdr->p_memsz,
        phdr->p_paddr, NUM_PAGES(phdr->p_vaddr, phdr->p_memsz));
}

static void map_page(page_table_t*, UINT64, UINT64, UINT64);

/* prepare boot page tables with Loader segments identity mapped and kernel
 * mapped to high half */
static page_table_t*
prepare_boot_page_tables(EFI_MEMORY_DESCRIPTOR *MemoryMap, UINT64 NumEntries,
                         const Elf64_Ehdr *ehdr, const Elf64_Phdr *phdrs)
{
    page_table_t *boot_page_table = (page_table_t*)allocate_page();

    /* identity map Loader segments */
    for (UINT64 i = 0; i < NumEntries; ++i) {
        EFI_MEMORY_DESCRIPTOR *Memory = &MemoryMap[i];
        if (Memory->Type != EfiLoaderCode && Memory->Type != EfiLoaderData)
            continue;

        for (UINT64 j = 0; j < Memory->NumberOfPages; ++j) {
            UINT64 Page = Memory->PhysicalStart + PAGE_SIZE * j;
            UINT64 Flags = 0;
            if (Memory->Type == EfiLoaderData)
                Flags |= PTE_RW;
            map_page(boot_page_table, Page, Page, Flags);
        }
    }

    /* map kernel to high half */
    for (Elf64_Half i = 0; i < ehdr->e_phnum; ++i) {
        const Elf64_Phdr *phdr = &phdrs[i];
        if (phdr->p_type != PT_LOAD)
            continue;
        Elf64_Xword num_pages = NUM_PAGES(phdr->p_vaddr, phdr->p_memsz);

        for (Elf64_Xword j = 0; j < num_pages; ++j) {
            UINT64 PPage = PAGE_BASE(phdr->p_paddr) + PAGE_SIZE * j;
            UINT64 VPage = PAGE_BASE(phdr->p_vaddr) + PAGE_SIZE * j;
            UINT64 Flags = 0;
            if (phdr->p_flags & PF_W)
                Flags |= PTE_RW;
            map_page(boot_page_table, PPage, VPage, Flags);
        }
    }

    return boot_page_table;
}

/* walk the page structures in order to map PPage to VPage */
static void
map_page(page_table_t *boot_page_table, UINT64 PPage, UINT64 VPage,
         UINT64 Flags)
{
    /* x86-64-system figure 4-8 */
    pte_t *pml4e = &(*boot_page_table)[PAGE_LEVEL_INDEX(VPage, 4)];
    if (!(*pml4e & PTE_P))
        *pml4e = allocate_page() | PTE_P | PTE_RW;

    page_table_t *pade_dir_ptr_table = (page_table_t*)(*pml4e & PTE_ADDR_MASK);
    pte_t *pdpte = &(*pade_dir_ptr_table)[PAGE_LEVEL_INDEX(VPage, 3)];
    if (!(*pdpte & PTE_P))
        *pdpte = allocate_page() | PTE_P | PTE_RW;

    page_table_t *page_directory = (page_table_t*)(*pdpte & PTE_ADDR_MASK);
    pte_t *pde = &(*page_directory)[PAGE_LEVEL_INDEX(VPage, 2)];
    if (!(*pde & PTE_P))
        *pde = allocate_page() | PTE_P | PTE_RW;

    page_table_t *page_table = (page_table_t*)(*pde & PTE_ADDR_MASK);
    pte_t *pte = &(*page_table)[PAGE_LEVEL_INDEX(VPage, 1)];
    if (*pte & PTE_P)
        EXIT_STATUS(EFI_ABORTED, L"remap 0x%lx", VPage);
    *pte = PPage | PTE_P | Flags;
}

/* allocate and zero one page of physical memory */
static UINT64
allocate_page(void)
{
    EFI_STATUS Status;
    UINT64 Page;
    if (_EFI_ERROR(Status = uefi_call_wrapper(BS->AllocatePages, 4,
            AllocateAnyPages, EfiLoaderData, 1, &Page)))
        EXIT_STATUS(Status, L"AllocatePages");
    memset((void*)Page, 0, PAGE_SIZE);
    return Page;
}

/* parse and complete filling out the memory map
 * *post ExitBootServices* */
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
    UINT64 NewLength = 1;

    for (UINT64 i = 1; i < *NumEntries; ++i) {
        EFI_MEMORY_DESCRIPTOR *Memory = &MemoryMap[i];
        /* uefi table 30: reclaim segment if not unusable by os */
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
