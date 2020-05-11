#include <efi.h>
#include <efilib.h>
#include <stdbool.h>
#include "efi-wrapper.h"
#include "readelf.h"
#include "elf.h"
#include "util.h"

bool
elf_read(void *fd, void *Buffer, UINT64 Offset, UINT64 Size)
{
    EFI_FILE_HANDLE File = fd;
    EFI_STATUS Status;

    if (_EFI_ERROR(Status = uefi_call_wrapper(File->SetPosition, 2,
            File, Offset))) {
        Print(L"SetPosition(%lu): %s\n", Offset, EFI_ERROR_STR(Status));
        return TRUE;
    }

    UINT64 ReadSize = Size;

    if (_EFI_ERROR(Status = uefi_call_wrapper(File->Read, 3,
            File, &ReadSize, Buffer))) {
        Print(L"Read(%lu): %s\n", Size, EFI_ERROR_STR(Status));
        return TRUE;
    }

    if (ReadSize != Size) {
        Print(L"read %lu bytes, expected %lu bytes", ReadSize, Size);
        return TRUE;
    }

    return FALSE;
}

void*
elf_alloc(UINT64 Size)
{
    return AllocatePool(Size);
}

void
elf_free(const void *ptr)
{
    FreePool((void*)ptr);
}

void
elf_on_not_elf(void *fd __unused)
{
    Print(L"Error: File is not an elf file\n");
}
