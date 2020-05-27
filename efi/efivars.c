/* this module provides a function to read over all of the EFI variables and
 * install a boot option for the operating system loader */
#include <efi.h>
#include <efilib.h>
#include "opsys/virtual-memory.h"
#include "efi-wrapper.h"
#include "efivars.h"
#include "util.h"

static void set_boot_variables(EFI_DEVICE_PATH*, UINT16);
static BOOLEAN print_next_efi_variable(CHAR16*, void*, EFI_GUID*);
static BOOLEAN StartsWith(const CHAR16*, const CHAR16*);
static BOOLEAN VariableIsBootOption(const CHAR16*);
static void print_guid(const CHAR16*, const EFI_GUID*);

/* read over all of the EFI variables, print them, and install a boot option for
 * the given device path */
void
enumerate_efi_vars(EFI_DEVICE_PATH *BootDevp, UINT16 DevpSize)
{
    CHAR16 *Name = (void*)allocate_page();
    void *Data = (void*)allocate_page();
    EFI_GUID Guid;
    BOOLEAN Boot0004Exists = FALSE;
    Print(L"efivars:\n");
    /* XXX: expansion
     * this should be generalized so that we allocate the next highest boot
     * option number available
     */

    while (print_next_efi_variable(Name, Data, &Guid)) {
        if (!Boot0004Exists && !StrCmp(Name, L"Boot0004"))
            Boot0004Exists = TRUE;
    }

    uefi_call_wrapper(BS->FreePages, 1, (UINT64)Name, 1);
    uefi_call_wrapper(BS->FreePages, 1, (UINT64)Data, 1);

    EFI_STATUS Status;
    UINT16 BootCurrent;
    UINT64 BootCurrentSize = sizeof(BootCurrent);
    if (_EFI_ERROR(Status = uefi_call_wrapper(RT->GetVariable, 5,
            L"BootCurrent", &gEfiGlobalVariableGuid, NULL, &BootCurrentSize,
            &BootCurrent)))
        EXIT_STATUS(Status, L"SetVariable");
    EFI_ASSERT(BootCurrentSize == sizeof(BootCurrent));

    /* install the boot option and read the variables again to make sure it
     * worked */
    /* (needs to be installed or reinstalled) and not on recursive call? */
    if ((!Boot0004Exists || BootCurrent != 4) && BootDevp && DevpSize) {
        set_boot_variables(BootDevp, DevpSize);
        enumerate_efi_vars(NULL, 0);
#ifdef _EFI_DEBUG
        /* we only want to debug from booting the loader directly, instead of
         * from the shell, because it's easier; it's to have a consistent load
         * address of the loader executable (to pass to gdb). */
        EXIT_STATUS(EFI_SUCCESS, L"installed boot option; reboot");
#endif
    }
}

/* Set boot option #4 to the given device path and update the boot order */
static void
set_boot_variables(EFI_DEVICE_PATH *BootDevp, UINT16 DevpSize)
{
    EFI_LOAD_OPTION Header = { LOAD_OPTION_ACTIVE, DevpSize };
    UINT64 LoadOptionSize;
    EFI_LOAD_OPTION *LoadOption = make_load_option(
        &Header, L"opsys loader", BootDevp, DevpSize, &LoadOptionSize);
    EFI_STATUS Status;
    if (_EFI_ERROR(Status = uefi_call_wrapper(RT->SetVariable, 5,
            L"Boot0004", &gEfiGlobalVariableGuid,
            EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | 
            EFI_VARIABLE_NON_VOLATILE,
            LoadOptionSize, LoadOption)))
        EXIT_STATUS(Status, L"SetVariable");
    FreePool(LoadOption);
    /* dummy option, opsys loader, UEFI shell. firmware will append other active
     * options not listed here. */
    UINT16 BootOrder[] = { 0, 4, 3 };
    if (_EFI_ERROR(Status = uefi_call_wrapper(RT->SetVariable, 5,
            L"BootOrder", &gEfiGlobalVariableGuid,
            EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | 
            EFI_VARIABLE_NON_VOLATILE,
            sizeof(BootOrder), BootOrder)))
        EXIT_STATUS(Status, L"SetVariable");
}

/* retrieve the next EFI variable, print its name, and print its information (if
 * we know how. returns if we should attempt to run this function again. */
static BOOLEAN
print_next_efi_variable(CHAR16 *Name, void *Data, EFI_GUID *Guid)
{
    EFI_STATUS Status;
    UINT64 NameSize = PAGE_SIZE;

    if (_EFI_ERROR(Status = uefi_call_wrapper(RT->GetNextVariableName, 3,
            &NameSize, Name, Guid))) {
        if (Status == EFI_NOT_FOUND)
            return FALSE;
        EXIT_STATUS(Status, L"GetNextVariableName");
    }

    UINT64 DataSize = PAGE_SIZE;
    UINT32 Attributes;

    if (_EFI_ERROR(Status = uefi_call_wrapper(RT->GetVariable, 5,
            Name, Guid, &Attributes, &DataSize, Data)))
        EXIT_STATUS(Status, L"GetVariable");

    Print(L"%c", Attributes & EFI_VARIABLE_NON_VOLATILE ? L'*' : ' ');
    print_guid(Name, Guid);

    if (!StrCmp(Name, L"BootOrder")) {
        UINT16 *BootOrder = Data;
        Print(L"BootOrder: ");
        for (UINT64 i = 0; i < DataSize / sizeof(UINT16); ++i)
            Print(L"%04x ", BootOrder[i]);
        Print(L"\n");
    } else if (!StrCmp(Name, L"BootCurrent")) {
        UINT16 *BootCurrent = Data;
        Print(L"BootCurrent: %04x\n", *BootCurrent);
    } else if (!StrCmp(Name, L"BootNext")) {
        UINT16 *BootNext = Data;
        Print(L"BootNext: %04x\n", *BootNext);
    } else if (VariableIsBootOption(Name)) {
        EFI_LOAD_OPTION *LoadOption = Data;
        const CHAR16 *Description =
            (void*)((UINT64)LoadOption + sizeof(*LoadOption));
        UINT64 DescriptionSize = StrSize(Description);
        Print(L"Description: %c%s\n",
            LoadOption->Attributes & LOAD_OPTION_ACTIVE ? L'*' : ' ',
            Description);
        EFI_DEVICE_PATH *FilePathList =
            (void*)((UINT64)Description + DescriptionSize);
        print_file_path(FilePathList);
        UINT8 *OptionalData = (void*)((UINT64)FilePathList
                                      + LoadOption->FilePathListLength);
        UINT64 OptionalDataSize =
            DataSize - ((UINT64)OptionalData - (UINT64)LoadOption);

        if (OptionalDataSize) {
            (void)OptionalData;
            /* for observing OptionalData
            BREAK(); 
            noop();
             */
        }
    }

    return TRUE;
}

#define IS_HEX_DIGIT(c) (IN_RANGE('0', 10, c) || IN_RANGE('a', 6, c))

/* uefi table 14: Boot#### where #### is a printed hex value with no 0x/h */
static BOOLEAN
VariableIsBootOption(const CHAR16 *Var)
{
    return StrLen(Var) == 8
        && StartsWith(Var, L"Boot")
        && IS_HEX_DIGIT(Var[4]) && IS_HEX_DIGIT(Var[5])
        && IS_HEX_DIGIT(Var[6]) && IS_HEX_DIGIT(Var[6]);
}

/* test if String starts with Prefix */
static BOOLEAN
StartsWith(const CHAR16 *String, const CHAR16 *Prefix)
{
    /* https://stackoverflow.com/a/26747997 */
    CHAR16 cs = 0, cp = 0;

    while ((cs = *String++) && (cp = *Prefix++)) {
        if (cs != cp)
            return FALSE;
    }

    return !cp;
}

void
print_file_path(EFI_DEVICE_PATH *FilePathList)
{
    CHAR16 *FilePathText;
    if (!(FilePathText = DevicePathToStr(FilePathList)))
        EXIT_STATUS(EFI_ABORTED, L"DevicePathToStr");
    Print(L"FilePath: %s\n", FilePathText);
    FreePool(FilePathText);
}

static void
print_guid(const CHAR16 *Name, const EFI_GUID *Guid)
{
    Print(L"%s(%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x)\n",
        Name, Guid->Data1, Guid->Data2, Guid->Data3,
        Guid->Data4[0], Guid->Data4[1], Guid->Data4[2], Guid->Data4[3],
        Guid->Data4[4], Guid->Data4[5], Guid->Data4[6], Guid->Data4[7]);
}
