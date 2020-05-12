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

    Elf64_Ehdr ehdr;
    Elf64_Phdr *phdrs = NULL;
    read_program_headers(File, &ehdr, &phdrs);
    load_elf_pages(File, &ehdr, phdrs);
    uefi_call_wrapper(File->Close, 1, File);
    uefi_call_wrapper(RootDir->Close, 1, RootDir);
    print_program_headers(&ehdr, phdrs);
    elf_free(phdrs);
    return EFI_SUCCESS;
}

static void print_program_header(const Elf64_Phdr*);

static void
print_program_headers(const Elf64_Ehdr *ehdr, const Elf64_Phdr *phdrs)
{
    if (!ehdr->e_phnum)
        return;
    Print(L"program headers:\n");
    Print(L"%-8s %5s %8s %8s %8s %8s %8s\n",
        L"type", L"flags", L"offset", L"filesz", L"paddr", L"vaddr", L"memsz");
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

    Print(L"%c%c%c   %8lx %8lx %8lx %8lx %8lx\n",
        phdr->p_flags & PF_R ? L'R' : L' ',
        phdr->p_flags & PF_W ? L'W' : L' ',
        phdr->p_flags & PF_X ? L'X' : L' ',
        phdr->p_offset, phdr->p_filesz,
        phdr->p_paddr,
        phdr->p_vaddr, phdr->p_memsz);
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
            if (tmp->p_type == PT_LOAD &&
                    IN_RANGE(tmp->p_vaddr, tmp->p_memsz, phdr->p_vaddr) &&
                    IN_RANGE(tmp->p_vaddr, tmp->p_memsz,
                             phdr->p_vaddr + phdr->p_memsz - 1)) {
                segment = tmp;
                break;
            }
        }

        phdr->p_paddr = segment
            ? PAGE_OFFSET(phdr->p_vaddr) + PAGE_BASE(segment->p_paddr)
            : 0;
    }
}
