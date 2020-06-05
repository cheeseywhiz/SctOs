#include <efi.h>
#include <efilib.h>

EFI_STATUS efi_main(EFI_HANDLE, EFI_SYSTEM_TABLE*);
static void id_reg_bug(void);
static void version_reg_bug(void);
void debug_breakpoint(void);
void noop(void);

EFI_STATUS
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS Status;
    InitializeLib(ImageHandle, SystemTable);
    EFI_LOADED_IMAGE *LoadedImage;
    Status = uefi_call_wrapper(BS->HandleProtocol, 3,
        ImageHandle, &LoadedImageProtocol, (void**)&LoadedImage);

    if (EFI_ERROR(Status)) {
        Print(L"handle LoadedImageProtocol: %d\n", Status);
        return Status;
    }

    Print(L"ImageBase: 0x%lx\n", LoadedImage->ImageBase);
    id_reg_bug();
    version_reg_bug();
    return EFI_SUCCESS;
}

static void
id_reg_bug(void)
{
    debug_breakpoint();
    /* if we step over this line, we should land at the correct landing spot,
     * but instead we land at the incorrect landing spot, because single
     * stepping has be turned off for some reason. */
    uint32_t lapic_id = *(uint32_t*)0xfee00020;
    noop(); /* correct landing spot */
    debug_breakpoint();
    noop(); /* incorrect landing spot */
    Print(L"0x%x\n", lapic_id);
}

static void
version_reg_bug(void)
{
    debug_breakpoint();
    /* likewise */
    uint32_t lapic_version  = *(uint32_t*)0xfee00030;
    /* note: the value is correctly read (0x50014 on both kvm and qemu cpu) */
    noop(); /* correct landing spot */
    debug_breakpoint();
    noop(); /* incorrect landing spot */
    Print(L"0x%x\n", lapic_version);
}

/* prevent dead code elimination with optimizations */
#define __weak __attribute__((weak))

__weak void
debug_breakpoint(void)
{
}

__weak void
noop(void)
{
    /* noop calls help clarify exactly where execution stops */
}
