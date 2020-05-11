/* this file loads the kernel executable into memory and runs it */
#include <efi.h>
#include <efilib.h>
#include "efi-wrapper.h"
#include "readelf.h"

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
