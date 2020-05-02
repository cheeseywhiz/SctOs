#pragma once
#include "elf.h"
#include <stdbool.h>

struct elf_symbol_table {
    const Elf64_Shdr *section;
    Elf64_Xword       n_symbols;
    const Elf64_Sym  *symbols;
    const char       *names;
};

struct elf_rel_table {
    const Elf64_Shdr *section;
    Elf64_Xword       n_relocations;
    const Elf64_Rela *relocations;
};

struct elf_file {
    Elf64_Ehdr               header;
    int                      fd;
    const Elf64_Phdr        *program_headers;
    const char              *fname;
    const Elf64_Shdr        *sections;
    const char              *section_names;
    const char              *interpreter;
    Elf64_Half               n_symbol_tables;
    struct elf_symbol_table *symbol_tables;
    Elf64_Half               n_rel_tables;
    struct elf_rel_table    *rel_tables;
};

const struct elf_file* readelf(const char *fname);
void free_elf_file(const struct elf_file*);

/* the user must define these */
void* elf_alloc(Elf64_Xword size);
#define ELF_FREE(p) elf_free((void*)p)
void elf_free(void*);
bool elf_open(struct elf_file*);
void elf_close(struct elf_file*);
void* elf_read(const struct elf_file*, void *buf, Elf64_Off offset,
               Elf64_Xword size);
/* the user may define this */
void elf_on_not_elf(const struct elf_file*);
