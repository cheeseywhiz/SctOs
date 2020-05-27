/* this file provides quality of life improvements on top of gnu-efi */
#include <efi.h>
#include <efilib.h>
#include "opsys/virtual-memory.h"
#include "efi-wrapper.h"
#include "util.h"

void
exit_status(const char *file, int line, EFI_STATUS Status, const CHAR16 *fmt,
            ...)
{
    Print(L"%s:%d: ", a2u(file), line);
    va_list ap;
    va_start(ap, fmt);
    VPrint(fmt, ap);
    va_end(ap);
    Print(L"\n%d %s\n", Status, EFI_ERROR_STR(Status));
    Exit(Status, 0, NULL);
    while (TRUE)
        ;
}

/* convert 8-bit string to 16-bit string */
CHAR16*
a2u(const char *a)
{
    static CHAR16 u[64];
    CHAR16 *u_p = u;
    while ((*u_p++ = (CHAR16)*a++))
        ;
    return u;
}

static UINT16 get_devp_size(EFI_DEVICE_PATH*);

/* append device paths Devp1 and Devp2, returning a new Devp buffer that must be
 * freed. Devp1 and Devp2 must both be valid, non-NULL device paths. */
EFI_DEVICE_PATH*
AppendPath(EFI_DEVICE_PATH *Devp1, EFI_DEVICE_PATH *Devp2, UINT16 *NewSize)
{
    /* this variable was reverse engineered from edk2 */
    static const EFI_DEVICE_PATH EndOfDevicePath = { 0x7f, 0xff, { 4, 0 } };
    UINT64 p1Size = get_devp_size(Devp1), p2Size = get_devp_size(Devp2);
    *NewSize = (UINT16)(p1Size + p2Size + sizeof(EFI_DEVICE_PATH));
    EFI_DEVICE_PATH *NewPath;
    if (!(NewPath = AllocatePool(*NewSize)))
        EXIT_STATUS(EFI_ABORTED, L"AllocatePool");
    EFI_DEVICE_PATH *End = NewPath;
    memcpy(End, Devp1, p1Size);
    End = (void*)((UINT64)End + p1Size);
    memcpy(End, Devp2, p2Size);
    End = (void*)((UINT64)End + p2Size);
    *End = EndOfDevicePath;
    return NewPath;
}

/* get the size of the device path, NOT including end-of-device-path. */
static UINT16
get_devp_size(EFI_DEVICE_PATH *Devp)
{
    UINT16 Size = 0;

    while (Devp->Type != 0x7f) {
        UINT16 Length = EFI_DEVICE_PATH_LENGTH(*Devp);
        Size += Length;
        Devp = (void*)((UINT64)Devp + Length);
    }

    return Size;
}

/* allocate and zero one page of physical memory */
UINT64
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

void
break_(void)
{
}
