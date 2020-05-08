/* this program reads its own _DYNAMIC table and prints out its dynamic symbol
 * table */
#include "elf.h"
#include "readelf.h"
#include "test-readelf.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

extern Elf64_Dyn _DYNAMIC[];

int
main(void)
{
    const Elf64_Sym *symbols = NULL;
    const char *strings = NULL;
    void *addr = NULL;

    for (const Elf64_Dyn *dyn = _DYNAMIC; dyn->d_tag != DT_NULL; ++dyn) {
        switch (dyn->d_tag) {
        case DT_SYMTAB:
            symbols = (const Elf64_Sym*)dyn->d_un.d_ptr;
            break;
        case DT_STRTAB:
            strings = (const char*)dyn->d_un.d_ptr;
            break;
        case DT_HASH:
            addr = (void*)dyn->d_un.d_ptr;
            break;
        default:
            break;
        }
    }

    if (!addr)
        return 1;
    struct elf_hash_table hash_table;
    init_hash_table(&hash_table, addr, symbols, strings);
    print_hash_table(&hash_table);
}
