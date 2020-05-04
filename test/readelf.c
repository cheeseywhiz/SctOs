/* TODO: port this program to my operating system */
#include "elf.h"
#include "util.h"
#include "string.h"
#include "readelf.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

static void print_elf_file(const struct elf_file*);

int
main(int argc, char *argv[])
{
    printf("sizeof(Elf64_Ehdr): %lu\n", sizeof(Elf64_Ehdr));
    printf("sizeof(Elf64_Phdr): %lu\n", sizeof(Elf64_Phdr));
    printf("sizeof(Elf64_Shdr): %lu\n", sizeof(Elf64_Shdr));
    printf("sizeof(Elf64_Sym):  %lu\n", sizeof(Elf64_Sym));
    printf("sizeof(Elf64_Rel):  %lu\n", sizeof(Elf64_Rel));
    printf("sizeof(Elf64_Rela): %lu\n", sizeof(Elf64_Rela));
    printf("section flags: ");
    for (size_t i = 0; i < ARRAY_LENGTH(elf_section_flags_str); ++i)
        printf("%s ", elf_section_flags_str[i]);
    puts("");
    int returncode = 0;

    for (int i = 1; i < argc; ++i) {
        puts("");
        const char *fname = argv[i];
        printf("file:\t\t\t%s\n", fname);
        bool bad = false;
        struct elf_file elf_file;
        init_elf_file(&elf_file);
        int fd;

        if ((fd = open(fname, O_RDONLY)) < 0) {
            fprintf(stderr, "open(\"%s\"): %d %s\n",
                    fname, errno, strerror(errno));
            bad = true;
            goto end;
        }

        if (readelf(&elf_file, &fd)) {
            bad = true;
            goto end;
        }

end:
        if (fd >= 0)
            close(fd);

        if (bad)
            ++returncode;
        else
            print_elf_file(&elf_file);

        free_elf_file(&elf_file);
    }

    return returncode;
}


void
elf_on_not_elf(const struct elf_file *elf_file __unused)
{
    fprintf(stderr, "file is not an ELF file\n");
}

void*
elf_alloc(Elf64_Xword size)
{
    void *buf;
    if (!(buf = malloc(size)))
        fprintf(stderr, "malloc(%lu): %d %s\n", size, errno, strerror(errno));
    return buf;
}

void
elf_free(const void *ptr)
{
    free((void*)ptr);
}

bool
elf_read(void *fd_, void *buf, Elf64_Off offset, Elf64_Xword size)
{
    if (offset > INT64_MAX) {
        // would interfere with off_t cast
        fprintf(stderr, "offset too large: %lu\n", offset);
        return true;
    }

    int fd = *(int*)fd_;

    if (lseek(fd, (off_t)offset, SEEK_SET) < 0) {
        fprintf(stderr, "lseek(%lu): %d %s\n", offset, errno, strerror(errno));
        return true;
    }

    if ((Elf64_Xword)read(fd, buf, size) != size) {
        fprintf(stderr, "read(%lu): %d %s\n", size,
                errno, strerror(errno));
        return true;
    }

    return false;
}

static void print_elf_header(const struct elf_file*);
static void print_program_headers(const struct elf_file*);
static void print_section_headers(const struct elf_file*);
static void print_symbol_tables(const struct elf_file*);
static void print_relocations(const struct elf_file*);

struct mmap_entry;
static struct mmap_entry* mmap_get(const struct elf_file*, Elf64_Xword*);
static void mmap_free(const struct mmap_entry*, Elf64_Xword);
static void mmap_sort(struct mmap_entry*, Elf64_Xword);
static void mmap_print(const struct mmap_entry*, Elf64_Xword);

static void
print_elf_file(const struct elf_file *elf_file)
{
    print_elf_header(elf_file);
    print_program_headers(elf_file);
    print_section_headers(elf_file);
    print_symbol_tables(elf_file);
    print_relocations(elf_file);
    if (elf_file->header->e_type != ET_EXEC
        && elf_file->header->e_type != ET_DYN)
        return;
    Elf64_Xword n_entries;
    struct mmap_entry *entries;
    if (!(entries = mmap_get(elf_file, &n_entries)))
        return;
    mmap_sort(entries, n_entries);
    mmap_print(entries, n_entries);
    mmap_free(entries, n_entries);
}

static void
print_elf_header(const struct elf_file *elf_file)
{
    const Elf64_Ehdr *header = elf_file->header;
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

static void
print_program_headers(const struct elf_file *elf_file)
{
    printf("\nprogram headers:\n");
    printf("%5s %-7s %5s %18s %18s %18s %18s %9s %s\n",
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
                && section->sh_addr + section->sh_size <= header->p_vaddr + header->p_memsz
        )
            printf("%s ", &elf_file->section_names[section->sh_name]);
    }

    puts("");
}

static void print_phdr_type(Elf64_Word type)
{
    if (type <= PT_MAX)
        printf("%-7s ", elf_segment_type_str[type]);
    else if (PT_LOOS <= type && type <= PT_HIOS)
        printf("%-7x ", type - PT_LOOS);
    else if (PT_LOPROC <= type && type <= PT_HIPROC)
        printf("%-7x ", type - PT_LOPROC);
    else
        printf("%-7u ", type);
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

static void
print_section_headers(const struct elf_file *elf_file)
{
    printf("\nsection headers:\n");
    printf("%-5s  %-16s %-18s %10s %10s %10s %18s %18s %9s %20s %10s %20s\n",
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

    printf("%10u %10u %s %#18lx %#18lx %#9lx %20lu %10lu %20lu\n",
        header->sh_link, header->sh_info, flags,
        header->sh_addr, header->sh_offset,
        header->sh_addralign,
        header->sh_size, header->sh_entsize,
        header->sh_entsize ? header->sh_size / header->sh_entsize : 0);
}

static void print_symbol(const struct elf_file*, const struct elf_symbol_table*,
                         Elf64_Xword);

static void
print_symbol_tables(const struct elf_file *elf_file)
{
    printf("\nsymbol tables:\n");
    printf("%20s  %-32s %-11s %-11s %-16s %18s %20s\n",
        "index", "name", "binding", "type", "section", "value", "size");
    for (Elf64_Half i = 0; i < elf_file->n_symbol_tables; ++i) {
        const struct elf_symbol_table *symbol_table =
            &elf_file->symbol_tables[i];
        for (Elf64_Xword j = 0; j < symbol_table->n_symbols; ++j)
            print_symbol(elf_file, symbol_table, j);
    }
}

static void
print_symbol(const struct elf_file *elf_file,
             const struct elf_symbol_table *symbol_table, Elf64_Xword j)
{
    const Elf64_Sym *symbol = &symbol_table->symbols[j];
    printf("%20lu%c %-32.32s ",
        j, j == symbol_table->section->sh_info ? '*' : ' ',
        &symbol_table->names[symbol->st_name]);
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

    const Elf64_Shdr *section_names_section =
        &elf_file->sections[symbol->st_shndx];

    if (symbol->st_shndx == SHN_ABS)
        printf("%-16s ", "ABS");
    else if (symbol->st_shndx == SHN_COMMON)
        printf("%-16s ", "COMMON");
    else
        printf("%-16.16s ",
            &elf_file->section_names[section_names_section->sh_name]);

    printf("%#18lx %20lu\n",
        symbol->st_value, symbol->st_size);
}

static const struct elf_symbol_table* get_symbol_table(const struct elf_file*,
                                                       Elf64_Half);
static void print_relocation(const struct elf_rel_table*,
                             const struct elf_symbol_table*, Elf64_Xword);

static void
print_relocations(const struct elf_file *elf_file)
{
    printf("\nrelocations:\n");
    printf(
        "%20s %-15s %18s %-32s %18s %19s\n",
        "index", "type", "virtual address", "symbol", "symbol value", "addend");

    for (Elf64_Half i = 0; i < elf_file->n_rel_tables; ++i) {
        const struct elf_rel_table *rel_table = &elf_file->rel_tables[i];
        const struct elf_symbol_table *symbol_table =
            get_symbol_table(elf_file, (Elf64_Half)rel_table->section->sh_link);
        printf(
            "relocation table: %s -> %s\n",
            &elf_file->section_names[rel_table->section->sh_name],
            &elf_file->section_names[elf_file->sections[
                rel_table->section->sh_info].sh_name]);
        for (Elf64_Xword j = 0; j < rel_table->n_relocations; ++j)
            print_relocation(rel_table, symbol_table, j);
    }
}

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
print_relocation(const struct elf_rel_table *rel_table,
                 const struct elf_symbol_table *symbol_table, Elf64_Xword j)
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

    printf(
        "%20lu %-15s %#18lx %-32.32s %#18lx %19s\n",
        j, elf_relocation_type_str[ELF64_R_TYPE(*rela)],
        rela->r_offset, symbol ? &symbol_table->names[symbol->st_name] : "",
        symbol ? symbol->st_value : 0, addend_hex);
}

struct mmap_entry {
    Elf64_Addr addr;
    Elf64_Addr end;
    const Elf64_Phdr *phdr;
    const char *section;
    const char *symbol;
};

static struct mmap_entry*
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
    struct mmap_entry *entries;
    if (!(entries = elf_alloc(sizeof(*entries) * *n_entries)))
        return NULL;

    for (Elf64_Half i = 0; i < *n_entries; ++i) {
        struct mmap_entry *entry = &entries[i];
        entry->addr = 0;
        entry->end = 0;
        entry->phdr = NULL;
        entry->section = NULL;
        entry->symbol = NULL;
    }

    Elf64_Xword entry_i = 0;

    for (Elf64_Half i = 1; i < elf_file->header->e_phnum; ++i) {
        const Elf64_Phdr *header = &elf_file->program_headers[i];
        struct mmap_entry *entry = &entries[entry_i++];
        entry->addr = header->p_vaddr;
        entry->end = header->p_vaddr + header->p_memsz;
        entry->phdr = header;
    }

    for (Elf64_Half i = 0; i < elf_file->header->e_shnum; ++i) {
        const Elf64_Shdr *header = &elf_file->sections[i];
        if (!(header->sh_flags & SHF_ALLOC))
            continue;
        struct mmap_entry *entry = &entries[entry_i++];
        entry->addr = header->sh_addr;
        entry->end = header->sh_addr + header->sh_size;
        entry->section = &elf_file->section_names[header->sh_name];
    }

    for (Elf64_Half i = 0; i < elf_file->n_symbol_tables; ++i) {
        const struct elf_symbol_table *symbol_table =
            &elf_file->symbol_tables[i];

        for (Elf64_Xword j = 0; j < symbol_table->n_symbols; ++j) {
            const Elf64_Sym *symbol = &symbol_table->symbols[j];
            struct mmap_entry *entry = &entries[entry_i++];
            entry->addr = symbol->st_value;
            entry->end = symbol->st_value + symbol->st_size;
            entry->symbol = &symbol_table->names[symbol->st_name];
        }
    }

    return entries;
}

static void
mmap_free(const struct mmap_entry *entries, Elf64_Xword n_entries __unused)
{
    elf_free(entries);
}

static void mmap_entry_swap(struct mmap_entry*, struct mmap_entry*);

/* insertion sort, stable */
static void
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

static void
mmap_print(const struct mmap_entry *entries, Elf64_Xword n_entries)
{
    printf("\nmmap:\n");

    for (Elf64_Xword i = 0; i < n_entries; ++i) {
        const struct mmap_entry *entry = &entries[i];
        if (entry->symbol && !*entry->symbol)
            continue;
        printf("%#18lx ", entry->addr);

        if (entry->end == entry->addr)
            printf("%18s ", "");
        else
            printf("%#18lx ", entry->end);

        if (entry->phdr) {
            print_phdr_type(entry->phdr->p_type);
            print_phdr_flags(entry->phdr->p_flags);
        } else {
            printf("%18s ", "");
        }

        printf("%-32.32s ", entry->section ? entry->section : "");
        printf("%s ", entry->symbol && *entry->symbol ? entry->symbol : "");
        puts("");
    }
}
