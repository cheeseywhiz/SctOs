/* this file loads the kernel executable into memory and runs it */
#include <efi.h>
#include <efilib.h>
#include "efi-wrapper.h"
#include "readelf.h"
#include "elf.h"

static void print_program_headers(const Elf64_Ehdr*, const Elf64_Phdr*);

EFI_STATUS
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_LOADED_IMAGE *LoadedImage = NULL;
    EFI_FILE_HANDLE RootDir = NULL, File = NULL;
    Elf64_Ehdr ehdr;
    const Elf64_Phdr *phdrs = NULL;
    InitializeLib(ImageHandle, SystemTable);

    if (_EFI_ERROR(Status = uefi_call_wrapper(BS->HandleProtocol, 3,
            ImageHandle, &LoadedImageProtocol, (void**)&LoadedImage)))
        goto finish;
    if (!(RootDir = LibOpenRoot(LoadedImage->DeviceHandle)))
        goto error;
    if (_EFI_ERROR(Status = uefi_call_wrapper(RootDir->Open, 5,
            RootDir, &File, L"\\opsys", EFI_FILE_MODE_READ, 0)))
        goto finish;
    if (read_program_headers(File, &ehdr, &phdrs))
        goto error;
    print_program_headers(&ehdr, phdrs);

finish:
    elf_free(phdrs);
    if (File)
        uefi_call_wrapper(File->Close, 1, File);
    if (RootDir)
        uefi_call_wrapper(RootDir->Close, 1, RootDir);

    if (_EFI_ERROR(Status))
        Print(L"%s\n", EFI_ERROR_STR(Status));
    else
        Print(L"%s\n", L"SUCCESS");

    return Status;

error:
    Status = EFI_ABORTED;
    goto finish;
}

static void print_program_header(const Elf64_Phdr*);

static void
print_program_headers(const Elf64_Ehdr *ehdr, const Elf64_Phdr *phdrs)
{
    if (!ehdr->e_phnum)
        return;
    Print(L"program headers:\n");
    Print(L"%-8s %5s %8s %8s %8s %8s\n",
        L"type", L"flags", L"offset", L"filesz", L"vaddr", L"memsz");

    for (Elf64_Half i = 0; i < ehdr->e_phnum; ++i)
        print_program_header(&phdrs[i]);
}

static CHAR16* a2u(const char*);

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

    Print(L"%c%c%c   %8lx %8lx %8lx %8lx\n",
        phdr->p_flags & PF_R ? L'R' : L' ',
        phdr->p_flags & PF_W ? L'W' : L' ',
        phdr->p_flags & PF_X ? L'X' : L' ',
        phdr->p_offset, phdr->p_filesz,
        phdr->p_vaddr, phdr->p_memsz);
}

/* convert 8-bit string to 16-bit string */
static CHAR16*
a2u(const char *a)
{
    static CHAR16 u[10];
    CHAR16 *u_p = u;
    while ((*u_p++ = (CHAR16)*a++))
        ;
    return u;
}
