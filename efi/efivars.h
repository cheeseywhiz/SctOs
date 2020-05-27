/* this module provides a function to read over all of the EFI variables and
 * install a boot option for the operating system loader */
#pragma once
#include <efi.h>
#include <efilib.h>

/* read over all of the EFI variables, print them, and install a boot option for
 * the given device path */
void enumerate_efi_vars(EFI_DEVICE_PATH*, UINT16);
void print_file_path(EFI_DEVICE_PATH*);
