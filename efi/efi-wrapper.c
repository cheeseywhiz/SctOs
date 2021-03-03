/* this file provides quality of life improvements on top of gnu-efi */
#include <efi.h>
#include <efilib.h>
#include "opsys/virtual-memory.h"
#include "efi-wrapper.h"
#include "util.h"

void
exit_status(const CHAR16 *file, int line, EFI_STATUS Status, const CHAR16 *fmt,
            ...)
{
    Print(L"%s:%d: ", file, line);
    va_list ap;
    va_start(ap, fmt);
    VPrint(fmt, ap);
    va_end(ap);
    Print(L"\n%d %s\n", Status, EFI_ERROR_STR(Status));
    Exit(Status, 0, NULL);
    __builtin_unreachable();
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
    EFI_ASSERT(Devp1 != NULL && Devp2 != NULL);
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

/* create a byte-packed EFI_LOAD_OPTION buffer according to uefi spec */
EFI_LOAD_OPTION*
make_load_option(const EFI_LOAD_OPTION *Header, const CHAR16 *Description,
                 EFI_DEVICE_PATH *Devp, UINT16 DevpSize, UINT64 *LoadOptionSize)
{
    UINT64 DescriptionSize = StrSize(Description);
    *LoadOptionSize = sizeof(*Header) + DescriptionSize + DevpSize;
    void *buf;
    if (!(buf = AllocatePool(*LoadOptionSize)))
        EXIT_STATUS(EFI_ABORTED, L"AllocatePool");
    void *end = buf;
    memcpy(end, Header, sizeof(*Header));
    end = (void*)((UINT64)end + sizeof(*Header));
    memcpy(end, Description, DescriptionSize);
    end = (void*)((UINT64)end + DescriptionSize);
    memcpy(end, Devp, DevpSize);
    return buf;
}

/* allocate and zero pages of physical memory */
UINT64
allocate_pages(UINT64 n_pages)
{
    EFI_STATUS Status;
    UINT64 Page;
    if (_EFI_ERROR(Status = uefi_call_wrapper(BS->AllocatePages, 4,
            AllocateAnyPages, EfiLoaderData, n_pages, &Page)))
        EXIT_STATUS(Status, L"AllocatePages");
    memset((void*)Page, 0, n_pages * PAGE_SIZE);
    return Page;
}

void
break_(void)
{
}
