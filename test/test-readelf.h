#pragma once
#include "elf.h"
#include "readelf.h"
#include <stdbool.h>

void print_elf_header(const Elf64_Ehdr*);
void print_program_headers(const struct elf_file*);
void print_section_headers(const struct elf_file*);
void print_dynamic(const struct elf_file*);
void print_symbol_tables(const struct elf_file*);
void print_symbol(const Elf64_Sym*, const char*);
void print_relocations(const struct elf_file*);

void print_hash_table(const struct elf_hash_table*);

enum mmap_entry_type {
    MET_NONE,
    MET_PHDR,
    MET_SHDR,
    MET_SYM,
    MET_REL,
};

struct mmap_entry {
    Elf64_Addr addr;
    Elf64_Addr end;
    enum mmap_entry_type type;
    union {
        const Elf64_Phdr *phdr;
        const char *section;
        const char *symbol;
        struct {
            const struct elf_rel_table    *table;
            const struct elf_symbol_table *sym_table;
            Elf64_Xword                    index;
        } r;
    } u;
};

/* generate the array of program headers, sections, symbols, and relocations
 * that form the memory map of the elf file's execution model. the length of the
 * array is output into n_entries. */
struct mmap_entry* mmap_get(const struct elf_file*, Elf64_Xword*);
void mmap_free(const struct mmap_entry*, Elf64_Xword);
/* sort the array by a stable insertion sort */
void mmap_sort(struct mmap_entry*, Elf64_Xword);
void mmap_print(const struct mmap_entry*, Elf64_Xword);
