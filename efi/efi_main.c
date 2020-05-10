#include <stddef.h>
#include <efi.h>
#include <efilib.h>
#include <efibind.h>
#include "util.h"
#include "efi-wrapper.h"

static CHAR16* a2u(char*, UINT64);

EFI_STATUS
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    InitializeLib(ImageHandle, SystemTable);
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_LOADED_IMAGE *LoadedImage = NULL;
    EFI_FILE_HANDLE RootDir = NULL, File = NULL;
    EFI_FILE_INFO *FileInfo = NULL;
    char *data = NULL;
    CHAR16 *udata = NULL;

    if (_EFI_ERROR(Status = uefi_call_wrapper(BS->HandleProtocol, 3,
            ImageHandle, &LoadedImageProtocol, (void**)&LoadedImage)))
        goto finish;
    if (!(RootDir = LibOpenRoot(LoadedImage->DeviceHandle)))
        goto error;
    if (_EFI_ERROR(Status = uefi_call_wrapper(RootDir->Open, 5,
            RootDir, &File, L"\\startup.nsh", EFI_FILE_MODE_READ, 0)))
        goto finish;
    if (!(FileInfo = LibFileInfo(File)))
        goto error;
    if (!(data = AllocatePool(FileInfo->FileSize + 1)))
        goto error;
    UINT64 FileSize = FileInfo->FileSize;
    if (_EFI_ERROR(Status = uefi_call_wrapper(File->Read, 3,
            File, &FileSize, data)))
        goto finish;
    if (FileSize != FileInfo->FileSize)
        goto error;
    data[FileSize] = 0;
    if (!(udata = a2u(data, FileSize)))
        goto error;
    Print(L"%s", udata);

finish:
    FreePool(udata);
    FreePool(data);
    FreePool(FileInfo);
    if (RootDir)
        uefi_call_wrapper(RootDir->Close, 1, File);

    if (_EFI_ERROR(Status))
        Print(L"%s\n\r", EFI_ERROR_STR(Status));
    else
        Print(L"%s\n\t", L"SUCCESS");

    return Status;

error:
    Status = EFI_ABORTED;
    goto finish;
}

/* gnu-efi/apps/t.c
 * convert 1-byte string into 2-byte string */
static CHAR16*
a2u(char *data, UINT64 length)
{
    CHAR16 *udata;
    if (!(udata = AllocatePool((length + 1) * sizeof(CHAR16))))
        return NULL;
    for (UINT64 i = 0; i < length; ++i)
        udata[i] = (CHAR16)data[i];
    udata[length] = 0;
    return udata;
}
