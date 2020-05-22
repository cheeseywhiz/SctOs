/* this file provides quality of life improvements on top of gnu-efi */
#include <efi.h>
#include <efilib.h>
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

void
break_(void)
{
}
