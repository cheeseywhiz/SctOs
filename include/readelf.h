#pragma once
#include "elf.h"
#include "util.h"
#include <stdbool.h>

/* the DT_HASH table containing chains of Elf64_Words gives us a hint that this
 * bounds the size of the symbol table */
struct elf_symbol_table {
    const Elf64_Shdr *section;
    Elf64_Word        n_symbols;
    const Elf64_Sym  *symbols;
    const char       *names;
};

struct elf_rel_table {
    const Elf64_Shdr *section;
    Elf64_Xword       n_relocations;
    const Elf64_Rela *relocations;
};

struct elf_dynamic {
    const Elf64_Dyn               *dynamic;
    const struct elf_symbol_table *symbol_table;
};

struct elf_file {
    const Elf64_Ehdr        *header;
    const Elf64_Phdr        *program_headers;
    const Elf64_Shdr        *sections;
    const char              *section_names;
    const char              *interpreter;
    struct elf_dynamic       dynamic;
    Elf64_Half               n_symbol_tables;
    struct elf_symbol_table *symbol_tables;
    Elf64_Half               n_rel_tables;
    struct elf_rel_table    *rel_tables;
};

/* init the elf_file with NULL/0 members */
void init_elf_file(struct elf_file*);
/* read each part of the elf file given by fd.
 * returns if an error occurred. */
bool readelf(struct elf_file*, void *fd);
void free_elf_file(const struct elf_file*);

/* XXX: the user must define these */
/* read size bytes at offset bytes from the beginning of file fd into buf.
 * return if an error occurred. */
bool elf_read(void *fd, void *buf, Elf64_Off offset, Elf64_Xword size);
/* allocate size uninitialized bytes */
__malloc void* elf_alloc(Elf64_Xword size);
/* free a pointer that originated from elf_alloc */
void elf_free(const void*);
/* XXX: the user may define this */
/* called when the given file is not an elf file. intended for logging. */
void elf_on_not_elf(void *fd);
