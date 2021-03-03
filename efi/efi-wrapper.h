/* this file provides quality of life improvements on top of gnu-efi */
#pragma once
#include <stddef.h>
#include <efi.h>
#include <efibind.h>
#include "util.h"
/* gnu-efi does not immediately wrap s in parens */
#define _EFI_ERROR(s) EFI_ERROR((s))
/* gnu-efi does not provide efi_main prototype for apps */
EFI_STATUS efi_main(EFI_HANDLE, EFI_SYSTEM_TABLE*);

/* https://stackoverflow.com/a/3291315 */
#define __(s) (L ## s)
/* turn a string literal into a long string literal */
#define _(s) __(s)

/* print a message to console and print the error status */
__noreturn void exit_status(const CHAR16*, int line, EFI_STATUS, const CHAR16*,
                            ...);
#define EXIT_STATUS(status, fmt, ...) \
    exit_status(_(__FILE__), __LINE__, (status), (fmt), ##__VA_ARGS__)
#define EFI_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            BREAK(); \
            EXIT_STATUS(EFI_ABORTED, L"Failed assertion: %s", _(#cond)); \
        } \
    } while (0)

/* convert 8-bit string to 16-bit string */
CHAR16* a2u(const char*a);

/* append device paths Devp1 and Devp2, returning a new Devp buffer that must be
 * freed. Devp1 and Devp2 must both be valid, non-NULL device paths. */
/* equivalent AppendDevicePath interface not provided by gnu-efi */
EFI_DEVICE_PATH* AppendPath(EFI_DEVICE_PATH*, EFI_DEVICE_PATH*, UINT16*);

/* uefi S3.1.3: not provided by gnu-efi */
typedef struct _EFI_LOAD_OPTION {
    UINT32 Attributes;
    UINT16 FilePathListLength;
    // byte packed buffer of the following:
    // CHAR16 Description[]; /* NULL terminated */
    // EFI_DEVICE_PATH FilePathList[];
    // UINT8 OptionalData[];
} __packed EFI_LOAD_OPTION;

#define LOAD_OPTION_ACTIVE 0x00000001

/* not standard, just handy */
#define EFI_DEVICE_PATH_LENGTH(devp) \
    (UINT16)(((devp).Length[1] << 8) | (devp).Length[0])

/* create a byte-packed EFI_LOAD_OPTION buffer according to uefi spec */
EFI_LOAD_OPTION* make_load_option(const EFI_LOAD_OPTION*, const CHAR16*,
                                  EFI_DEVICE_PATH*, UINT16, UINT64*);

/* allocate and zero one page of physical memory */
UINT64 allocate_pages(UINT64 n_pages);

#define _ERROR_STR(suffix) [(EFI_ ## suffix) & ~EFI_ERROR_MASK] = _(#suffix)
#define EFI_ERROR_STR(status) (efi_error_str[(status) & ~EFI_ERROR_MASK])

/* uefi table 258 page 2210 / gnu-efi/inc/efierr.h */
static const CHAR16 *const efi_error_str[] = {
    _ERROR_STR(SUCCESS),
    _ERROR_STR(LOAD_ERROR),
    _ERROR_STR(INVALID_PARAMETER),
    _ERROR_STR(UNSUPPORTED),
    _ERROR_STR(BAD_BUFFER_SIZE),
    _ERROR_STR(BUFFER_TOO_SMALL),
    _ERROR_STR(NOT_READY),
    _ERROR_STR(DEVICE_ERROR),
    _ERROR_STR(WRITE_PROTECTED),
    _ERROR_STR(OUT_OF_RESOURCES),
    _ERROR_STR(VOLUME_CORRUPTED),
    _ERROR_STR(VOLUME_FULL),
    _ERROR_STR(NO_MEDIA),
    _ERROR_STR(MEDIA_CHANGED),
    _ERROR_STR(NOT_FOUND),
    _ERROR_STR(ACCESS_DENIED),
    _ERROR_STR(NO_RESPONSE),
    _ERROR_STR(NO_MAPPING),
    _ERROR_STR(TIMEOUT),
    _ERROR_STR(NOT_STARTED),
    _ERROR_STR(ALREADY_STARTED),
    _ERROR_STR(ABORTED),
    _ERROR_STR(ICMP_ERROR),
    _ERROR_STR(TFTP_ERROR),
    _ERROR_STR(PROTOCOL_ERROR),
    _ERROR_STR(INCOMPATIBLE_VERSION),
    _ERROR_STR(SECURITY_VIOLATION),
    _ERROR_STR(CRC_ERROR),
    _ERROR_STR(END_OF_MEDIA),
    _ERROR_STR(END_OF_FILE),
    _ERROR_STR(INVALID_LANGUAGE),
    _ERROR_STR(COMPROMISED_DATA),
};

/* uefi AllocatePages related definitions page 162 */
#define _MT_STR(type) [Efi ## type] = _(#type)
static const CHAR16 *const efi_memory_type_str[] = {
    _MT_STR(ReservedMemoryType),
    _MT_STR(LoaderCode),
    _MT_STR(LoaderData),
    _MT_STR(BootServicesCode),
    _MT_STR(BootServicesData),
    _MT_STR(RuntimeServicesCode),
    _MT_STR(RuntimeServicesData),
    _MT_STR(ConventionalMemory),
    _MT_STR(UnusableMemory),
    _MT_STR(ACPIReclaimMemory),
    _MT_STR(ACPIMemoryNVS),
    _MT_STR(MemoryMappedIO),
    _MT_STR(MemoryMappedIOPortSpace),
    _MT_STR(PalCode),
};
