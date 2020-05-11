/* this file contains facilities for printing the information found in a
 * struct elf_file produced by the lib readelf function */
#include "elf.h"
#include "util.h"
#include "string.h"
#include "readelf.h"
#include "print-elf.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

void
print_elf_header(const Elf64_Ehdr *header)
{
    puts("elf header:");
    printf("\tmagic:\t\t%#x %c%c%c\n",
        header->e_ident[EI_MAG0], header->e_ident[EI_MAG1],
        header->e_ident[EI_MAG2], header->e_ident[EI_MAG3]);
    printf("\tclass:\t\t%s\n", elf_class_str[header->e_ident[EI_CLASS]]);
    printf("\tendianness:\t%s\n",
        elf_endianness_str[header->e_ident[EI_ENDIANNESS]]);
    printf("\tversion:\t%s\n", elf_version_str[header->e_ident[EI_VERSION]]);
    printf("\tosabi:\t\t%s\n", elf_osabi_str[header->e_ident[EI_OSABI]]);
    printf("\ttype:\t\t%s\n", elf_type_str[header->e_type]);
    printf("\tmachine:\t%s\n", elf_machine_str[header->e_machine]);
    printf("\tversion:\t%s\n", elf_version_str[header->e_version]);
    printf("\tentry:\t\t%#lx\n", header->e_entry);
    printf("\tflags:\t\t0x%08x\n", header->e_flags);
    printf("\tehsize:\t\t%u\n", header->e_ehsize);
    printf("\tphoff:\t\t%#lx\n", header->e_phoff);
    printf("\tphentsize:\t%u\n", header->e_phentsize);
    printf("\tphnum:\t\t%u\n", header->e_phnum);
    printf("\tshoff:\t\t%#lx\n", header->e_shoff);
    printf("\tshentsize:\t%u\n", header->e_shentsize);
    printf("\tshnum:\t\t%u\n", header->e_shnum);
    printf("\tshstrndx:\t%u\n", header->e_shstrndx);
}

static void print_program_header(const struct elf_file*, const Elf64_Phdr*);

void
print_program_headers(const struct elf_file *elf_file)
{
    if (!elf_file->header->e_phnum)
        return;
    printf("\nprogram headers:\n");
    printf("%5s %-8s %5s %18s %18s %18s %18s %9s %s\n",
        "index", "type", "flags", "file offset", "size in file",
        "virtual address", "size in memory", "align", "sections");

    for (Elf64_Half i = 0; i < elf_file->header->e_phnum; ++i) {
        printf("%5u ", i);
        const Elf64_Phdr *header = &elf_file->program_headers[i];
        print_program_header(elf_file, header);
    }
}

static void print_phdr_type(Elf64_Word);
static void print_phdr_flags(Elf64_Word);

static void
print_program_header(const struct elf_file *elf_file, const Elf64_Phdr *header)
{
    print_phdr_type(header->p_type);
    print_phdr_flags(header->p_flags);

    printf("  %#18lx %#18lx %#18lx %#18lx %#9lx ",
        header->p_offset, header->p_filesz,
        header->p_vaddr, header->p_memsz,
        header->p_align);

    for (Elf64_Half i = 1; i < elf_file->header->e_shnum; ++i) {
        const Elf64_Shdr *section = &elf_file->sections[i];

        if (section->sh_flags & SHF_ALLOC
                && header->p_vaddr <= section->sh_addr
                && section->sh_addr < header->p_vaddr + header->p_memsz
                && header->p_vaddr <= section->sh_addr + section->sh_size
                && section->sh_addr + section->sh_size
                    <= header->p_vaddr + header->p_memsz)
            printf("%s ", &elf_file->section_names[section->sh_name]);
    }

    puts("");
}

static void print_phdr_type(Elf64_Word type)
{
    if (type < PT_NUM)
        printf("%-8s ", elf_segment_type_str[type]);
    else if (PT_GNU_STACK <= type && type <= PT_GNU_RELRO)
        printf("%-8s ", elf_segment_type_str[PT_NUM + type - PT_GNU_STACK]);
    else
        printf("%-8x ", type);
}

static void
print_phdr_flags(Elf64_Word flags)
{
    printf("%c%c%c ",
        flags & PF_R ? 'R' : ' ',
        flags & PF_W ? 'W' : ' ',
        flags & PF_X ? 'X' : ' ');
}

static void print_section_header(const struct elf_file*, const Elf64_Shdr*);

void
print_section_headers(const struct elf_file *elf_file)
{
    printf("\nsection headers:\n");
    printf("section flags: ");
    for (size_t i = 0; i < ARRAY_LENGTH(elf_section_flags_str); ++i)
        printf("%s ", elf_section_flags_str[i]);
    puts("");
    printf("%-5s  %-16s %-18s %10s %10s %10s %18s %18s %9s %18s %10s %20s\n",
        "index", "name", "type", "link", "info", "flags", "virtual address",
        "file offset", "alignment", "size in file", "entry size",
        "num elements");

    for (Elf64_Half i = SHN_BEGIN; i < elf_file->header->e_shnum; ++i) {
        printf("%5u%c ", i, i == elf_file->header->e_shstrndx ? '*' : ' ');
        const Elf64_Shdr *header = &elf_file->sections[i];
        print_section_header(elf_file, header);
    }

    if (elf_file->interpreter)
        printf("interpreter: %s\n", elf_file->interpreter);
}

#define FLAG(flag) \
    (header->sh_flags & (flag) ? (#flag)[4] : ' ')

static void
print_section_header(const struct elf_file *elf_file, const Elf64_Shdr *header)
{
    char flags[] = {
        FLAG(SHF_COMPRESSED),
        FLAG(SHF_TLS),
        FLAG(SHF_GROUP),
        FLAG(SHF_OS_NONCONFORMING),
        FLAG(SHF_LINK_ORDER),
        FLAG(SHF_STRINGS),
        FLAG(SHF_MERGE),
        FLAG(SHF_EXECINSTR),
        FLAG(SHF_ALLOC),
        FLAG(SHF_WRITE),
        0
    };
    printf("%-16.16s ", &elf_file->section_names[header->sh_name]);
    Elf64_Word type = header->sh_type;

    if (type <= SHT_NUM)
        printf("%-18s ", elf_section_type_str[type]);
    else if (SHT_LOOS <= type && type <= SHT_HIOS)
        printf("%-18s ", "OS-specific");
    else if (SHT_LOPROC <= type && type <= SHT_HIPROC)
        printf("%-18s ", "Processor-specific");
    else
        printf("%#-10x         ", type);

    printf("%10u %10u %s %#18lx %#18lx %#9lx %#18lx %10lu %20lu\n",
        header->sh_link, header->sh_info, flags,
        header->sh_addr, header->sh_offset,
        header->sh_addralign,
        header->sh_size, header->sh_entsize,
        header->sh_entsize ? header->sh_size / header->sh_entsize : 0);
}

void
print_dynamic(const struct elf_file *elf_file)
{
    if (!elf_file->dynamic.dynamic)
        return;
    printf("\ndynamic:\n");

    for (const Elf64_Dyn *dyn = elf_file->dynamic.dynamic;
         dyn->d_tag != DT_NULL; ++dyn) {
        if (dyn->d_tag <= DT_MAX)
            printf("%-12s ", elf_dyn_tag_str[dyn->d_tag]);
        else if (DT_LOOS <= dyn->d_tag && dyn->d_tag <= DT_HIOS)
            printf("%#-12lx ", dyn->d_tag);
        else
            printf("%#-12lx ", dyn->d_tag);

        switch (dyn->d_tag) {
        case DT_NEEDED:
        case DT_SONAME:
        case DT_RPATH:
            printf("%s\n",
                &elf_file->dynamic.symbol_table->names[dyn->d_un.d_val]);
            break;
        case DT_PLTRELSZ:
        case DT_RELASZ:
        case DT_RELAENT:
        case DT_STRSZ:
        case DT_SYMENT:
        case DT_RELSZ:
        case DT_RELENT:
        case DT_PLTREL:
        case DT_INIT_ARRAYSZ:
        case DT_FINI_ARRAYSZ:
            printf("val: %#lx\n", dyn->d_un.d_val);
            break;
        case DT_PLTGOT:
        case DT_HASH:
        case DT_STRTAB:
        case DT_SYMTAB:
        case DT_RELA:
        case DT_INIT:
        case DT_FINI:
        case DT_REL:
        case DT_JMPREL:
        case DT_INIT_ARRAY:
        case DT_FINI_ARRAY:
            printf("ptr: %#lx\n", dyn->d_un.d_ptr);
            break;
        case DT_SYMBOLIC:
        case DT_TEXTREL:
        case DT_DEBUG:
        case DT_BIND_NOW:
            /* test for presence only, no value */
            printf("\n");
            break;
        default:
            printf("unknown: %#lx\n", dyn->d_un.d_val);
            break;
        }
    }
}

void
print_symbol_tables(const struct elf_file *elf_file)
{
    if (!elf_file->n_symbol_tables)
        return;
    printf("\nsymbol tables:\n");

    for (Elf64_Half i = 0; i < elf_file->n_symbol_tables; ++i) {
        const struct elf_symbol_table *symbol_table =
            &elf_file->symbol_tables[i];
        if (symbol_table->n_symbols <= 1)
            continue;
        printf("%s\n",
            &elf_file->section_names[symbol_table->section->sh_name]);
        printf("%11s %-32s %-11s %-11s %18s %18s %-16s\n",
            "index", "name", "binding", "type", "value", "size", "section");

        for (Elf64_Word j = 1; j < symbol_table->n_symbols; ++j) {
            const Elf64_Sym *symbol = &symbol_table->symbols[j];
            printf("%10u%c ",
                j, j == symbol_table->section->sh_info ? '*' : ' ');
            print_symbol(symbol, symbol_table->names);
            const Elf64_Shdr *section_names_section =
                &elf_file->sections[symbol->st_shndx];
            printf("%-16.16s\n",
                symbol->st_shndx == SHN_ABS ? "ABS"
                : symbol->st_shndx == SHN_COMMON ? "COMMON"
                : &elf_file->section_names[section_names_section->sh_name]);
        }
    }
}

void
print_symbol(const Elf64_Sym *symbol, const char *names)
{
    printf("%-32.32s ", &names[symbol->st_name]);
    unsigned char binding = ELF_SYMBOL_BINDING(*symbol);

    if (binding < STB_NUM)
        printf("%-11s ", elf_symbol_binding_str[binding]);
    else if (STB_LOOS <= binding && binding <= STB_HIOS)
        printf("%-11s ", "OS-specific");
    else if (STB_LOPROC <= binding && binding <= STB_HIPROC)
        printf("%-11s ", "Proc");

    unsigned char type = ELF_SYMBOL_TYPE(*symbol);

    if (type < STT_NUM)
        printf("%-11s ", elf_symbol_type_str[type]);
    else if (STT_LOOS <= type && type <= STT_HIOS)
        printf("%-11s ", "OS-specific");
    else if (STT_LOPROC <= type && type <= STT_HIPROC)
        printf("%-11s ", "Proc");

    printf("%#18lx %#18lx ", symbol->st_value, symbol->st_size);
}

static const struct elf_symbol_table* get_symbol_table(const struct elf_file*,
                                                       Elf64_Half);
static void print_relocation(const struct elf_rel_table*,
                             const struct elf_symbol_table*, Elf64_Xword, bool);

void
print_relocations(const struct elf_file *elf_file)
{
    if (!elf_file->n_rel_tables)
        return;
    printf("\nrelocations:\n");

    for (Elf64_Half i = 0; i < elf_file->n_rel_tables; ++i) {
        const struct elf_rel_table *rel_table = &elf_file->rel_tables[i];
        if (!rel_table->n_relocations)
            continue;
        const struct elf_symbol_table *symbol_table =
            get_symbol_table(elf_file, (Elf64_Half)rel_table->section->sh_link);
        printf("reloation table: %s -> %s\n",
            &elf_file->section_names[rel_table->section->sh_name],
            &elf_file->section_names[elf_file->sections[
                rel_table->section->sh_info].sh_name]);
        printf("%20s %-15s %18s %-32s %18s %19s\n",
            "index", "type", "virtual address", "symbol", "symbol value",
            "addend");
        for (Elf64_Xword j = 0; j < rel_table->n_relocations; ++j)
            print_relocation(rel_table, symbol_table, j, true);
    }
}

/* get the symbol table item that has the given section index as its section */
static const struct elf_symbol_table*
get_symbol_table(const struct elf_file *elf_file, Elf64_Half ndx)
{
    const Elf64_Shdr *section = &elf_file->sections[ndx];

    for (Elf64_Half i = 0; i < elf_file->n_symbol_tables; ++i) {
        const struct elf_symbol_table *symbol_table =
            &elf_file->symbol_tables[i];
        if (section == symbol_table->section)
            return symbol_table;
    }

    return NULL;
}

static void
print_relocation(
    const struct elf_rel_table *rel_table,
    const struct elf_symbol_table *symbol_table, Elf64_Xword j, bool table_mode)
{
    const Elf64_Rela *rela = &rel_table->relocations[j];
    const Elf64_Sym *symbol = symbol_table
        ? &symbol_table->symbols[ELF64_R_SYM(*rela)]
        : NULL;
    /* 1 sign + 2 alt form + 16 hex + 1 null = 20 chars */
    char addend_hex[20];
    sprintf(addend_hex, "%c%#lx",
            rela->r_addend < 0 ? '-' : ' ',
            rela->r_addend < 0 ? -(Elf64_Xword)rela->r_addend
                               : (Elf64_Xword)rela->r_addend);

    if (table_mode)
        printf("%20lu ", j);
    printf("%-15s ", elf_relocation_type_str[ELF64_R_TYPE(*rela)]);
    if (table_mode)
        printf("%#18lx ", rela->r_offset);
    printf("%-32.32s %#18lx %19s\n",
        symbol ? &symbol_table->names[symbol->st_name] : "",
        symbol ? symbol->st_value : 0, addend_hex);
}

void
print_hash_table(const struct elf_hash_table *hash_table)
{
    for (Elf64_Xword i = 0; i < hash_table->nbucket; ++i) {
        Elf64_Word j;
        if (!(j = hash_table->buckets[i]))
            continue;
        print_symbol(&hash_table->symbols[j], hash_table->strings);
        printf("\n");

        for (const Elf64_Word *k = &hash_table->chains[i]; *k; ++k) {
            print_symbol(&hash_table->symbols[*k], hash_table->strings);
            printf("\n");
        }
    }
}

/* generate the array of program headers, sections, symbols, and relocations
 * that form the memory map of the elf file's execution model. the length of the
 * array is output into n_entries. */
struct mmap_entry*
mmap_get(const struct elf_file *elf_file, Elf64_Xword *n_entries)
{
    *n_entries = elf_file->header->e_phnum
        ? (Elf64_Xword)(elf_file->header->e_phnum - 1)
        : 0;

    for (Elf64_Half i = 0; i < elf_file->header->e_shnum; ++i) {
        if (elf_file->sections[i].sh_flags & SHF_ALLOC)
            ++*n_entries;
    }

    for (Elf64_Half i = 0; i < elf_file->n_symbol_tables; ++i)
        *n_entries += elf_file->symbol_tables[i].n_symbols;
    for (Elf64_Half i = 0; i < elf_file->n_rel_tables; ++i)
        *n_entries += elf_file->rel_tables[i].n_relocations;
    struct mmap_entry *entries;
    if (!(entries = elf_alloc(sizeof(*entries) * *n_entries)))
        return NULL;

    for (Elf64_Half i = 0; i < *n_entries; ++i) {
        struct mmap_entry *entry = &entries[i];
        entry->addr = 0;
        entry->end = 0;
        entry->type = MET_NONE;
    }

    Elf64_Xword entry_i = 0;

    for (Elf64_Half i = 1; i < elf_file->header->e_phnum; ++i) {
        const Elf64_Phdr *header = &elf_file->program_headers[i];
        struct mmap_entry *entry = &entries[entry_i++];
        entry->addr = header->p_vaddr;
        entry->end = header->p_vaddr + header->p_memsz;
        entry->type = MET_PHDR;
        entry->u.phdr = header;
    }

    for (Elf64_Half i = 0; i < elf_file->header->e_shnum; ++i) {
        const Elf64_Shdr *header = &elf_file->sections[i];
        if (!(header->sh_flags & SHF_ALLOC))
            continue;
        struct mmap_entry *entry = &entries[entry_i++];
        entry->addr = header->sh_addr;
        entry->end = header->sh_addr + header->sh_size;
        entry->type = MET_SHDR;
        entry->u.section = &elf_file->section_names[header->sh_name];
    }

    for (Elf64_Half i = 0; i < elf_file->n_symbol_tables; ++i) {
        const struct elf_symbol_table *symbol_table =
            &elf_file->symbol_tables[i];

        for (Elf64_Word j = 0; j < symbol_table->n_symbols; ++j) {
            const Elf64_Sym *symbol = &symbol_table->symbols[j];
            struct mmap_entry *entry = &entries[entry_i++];
            entry->addr = symbol->st_value;
            entry->end = symbol->st_value + symbol->st_size;
            entry->type = MET_SYM;
            entry->u.symbol = &symbol_table->names[symbol->st_name];
        }
    }

    for (Elf64_Half i = 0; i < elf_file->n_rel_tables; ++i) {
        const struct elf_rel_table *rel_table = &elf_file->rel_tables[i];
        const struct elf_symbol_table *rel_sym_table =
            get_symbol_table(elf_file, (Elf64_Half)rel_table->section->sh_link);

        for (Elf64_Xword j = 0; j < rel_table->n_relocations; ++j) {
            const Elf64_Rela *rela = &rel_table->relocations[j];
            struct mmap_entry *entry = &entries[entry_i++];
            entry->addr = entry->end = rela->r_offset;
            entry->type = MET_REL;
            entry->u.r.table = rel_table;
            entry->u.r.sym_table = rel_sym_table;
            entry->u.r.index = j;
        }
    }

    return entries;
}

void
mmap_free(const struct mmap_entry *entries, Elf64_Xword n_entries __unused)
{
    elf_free(entries);
}

static void mmap_entry_swap(struct mmap_entry*, struct mmap_entry*);

/* sort the array by a stable insertion sort */
void
mmap_sort(struct mmap_entry* entries, Elf64_Xword n_entries)
{
    for (Elf64_Xword i = 1; i < n_entries; ++i) {
        for (Elf64_Xword j = i; j; --j) {
            struct mmap_entry *here = &entries[j], *back = &entries[j - 1];
            if (!(back->addr > here->addr))
                break;
            mmap_entry_swap(here, back);
        }
    }
}

static void
mmap_entry_swap(struct mmap_entry *e1, struct mmap_entry *e2)
{
    struct mmap_entry tmp = *e1;
    *e1 = *e2;
    *e2 = tmp;
}

void
mmap_print(const struct mmap_entry *entries, Elf64_Xword n_entries)
{
    printf("\nmmap:\n");

    for (Elf64_Xword i = 0; i < n_entries; ++i) {
        const struct mmap_entry *entry = &entries[i];
        if (entry->type == MET_NONE
            || (entry->type == MET_SYM && !*entry->u.symbol))
            continue;
        printf("%#18lx ", entry->addr);

        if (entry->end == entry->addr)
            printf("%18s ", "");
        else
            printf("%#18lx ", entry->end);

        if (entry->type == MET_PHDR) {
            print_phdr_type(entry->u.phdr->p_type);
            print_phdr_flags(entry->u.phdr->p_flags);
        } else {
            printf("%12s ", "");
        }

        printf("%-32.32s ", entry->type == MET_SHDR ? entry->u.section : "");
        printf("%-32.32s ", entry->type == MET_SYM ? entry->u.symbol : "");

        if (entry->type == MET_REL)
            print_relocation(entry->u.r.table, entry->u.r.sym_table,
                             entry->u.r.index, false);
        else
            puts("");
    }
}
