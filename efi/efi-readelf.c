#include <efi.h>
#include <efilib.h>
#include <stdbool.h>
#include <stdarg.h>
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
            File, Offset)))
        EXIT_STATUS(Status, L"SetPosition(%lu)", Offset);
    UINT64 ReadSize = Size;
    if (_EFI_ERROR(Status = uefi_call_wrapper(File->Read, 3,
            File, &ReadSize, Buffer)))
        EXIT_STATUS(Status, L"Read(%lu)", Size);
    if (ReadSize != Size)
        EXIT_STATUS(Status, L"read %lu bytes, expected %lu bytes",
            ReadSize, Size);
    return FALSE;
}

void*
elf_alloc(UINT64 Size)
{
    void *pool;
    if (!(pool = AllocatePool(Size)))
        EXIT_STATUS(EFI_ABORTED, L"AllocatePool", EFI_ABORTED);
    return pool;
}

void
elf_free(const void *ptr)
{
    FreePool((void*)ptr);
}

void
elf_on_not_elf(void *fd __unused)
{
    EXIT_STATUS(EFI_ABORTED, L"Error: File is not an elf file");
}
