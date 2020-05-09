/* thie file provides quality of life improvements on top of gnu-efi */
#pragma once
#include <efi.h>
/* gnu-efi does not immediately wrap s in parens */
#define _EFI_ERROR(s) EFI_ERROR((s))
/* gnu-efi does not provide efi_main prototype for apps */
EFI_STATUS efi_main(EFI_HANDLE, EFI_SYSTEM_TABLE*);
