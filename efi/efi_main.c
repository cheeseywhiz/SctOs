#include <stddef.h>
#include <efi.h>
#include <efilib.h>
#include <efibind.h>
#include "util.h"
#include "efi-wrapper.h"

EFI_STATUS
efi_main(EFI_HANDLE ImageHandle __unused, EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS Status;
    ST = SystemTable;
    if (_EFI_ERROR(Status =
            ST->ConOut->OutputString(ST->ConOut, L"Hello World\n\r")))
        return Status;
    if (_EFI_ERROR(Status = ST->ConIn->Reset(ST->ConIn, FALSE)))
        return Status;
    return Status;
}
