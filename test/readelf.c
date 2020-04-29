/* TODO: port this program to my operating system */
#include "elf.h"
#include "util.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

/* returns if an error occurred */
static struct elf_file* readelf(const char*);
static void free_elf_file(struct elf_file*);
static void print_elf_file(const struct elf_file*);

int
main(int argc, char *argv[]) {
    printf("sizeof(Elf64_Ehdr): %lu\n", sizeof(Elf64_Ehdr));
    printf("sizeof(Elf64_Shdr): %lu\n", sizeof(Elf64_Shdr));
    printf("sizeof(Elf64_Sym):  %lu\n", sizeof(Elf64_Sym));
    printf("flags: ");
    for (size_t i = 0; i < ARRAY_LENGTH(elf_section_flags_str); ++i)
        printf("%s ", elf_section_flags_str[i]);
    puts("");
    int returncode = 0;

    for (int i = 1; i < argc; ++i) {
        puts("");
        const char *fname = argv[i];
        printf("file:\t\t\t%s\n", fname);
        struct elf_file *elf_file;

        if (!(elf_file = readelf(fname))) {
            returncode = 1;
            continue;
        }

        print_elf_file(elf_file);
        free_elf_file(elf_file);
    }

    return returncode;
}

/* returns if an error occurred */
static bool read_sections(struct elf_file*);
static bool read_symbol_tables(struct elf_file*);
static const void* read_section_data(const struct elf_file*, Elf64_Half);

static struct elf_file*
readelf(const char *fname) {
    bool bad = false;
    struct elf_file *elf_file = NULL;

    if (!(elf_file = malloc(sizeof(*elf_file)))) {
        fprintf(stderr, "malloc(elf_file): %d %s\n", errno, strerror(errno));
        bad = true;
        goto end;
    }

    elf_file->fname = fname;
    elf_file->sections = NULL;
    elf_file->section_names = NULL;
    elf_file->n_symbol_tables = 0;
    elf_file->symbol_tables = NULL;

    if ((elf_file->fd = open(elf_file->fname, O_RDONLY)) < 0) {
        fprintf(stderr, "open(\"%s\");\n", elf_file->fname);
        bad = true;
        goto end;
    }

    if ((size_t)read(elf_file->fd, &elf_file->header, sizeof(elf_file->header))
            != sizeof(elf_file->header)) {
        fprintf(stderr, "read(elf_header): %d %s\n", errno, strerror(errno));
        bad = true;
        goto end;
    }

    if (!ELF_VERIFY_MAGIC(elf_file->header)) {
        fprintf(stderr, "file is not ELF\n");
        bad = true;
        goto end;
    }

    if (read_sections(elf_file)) {
        bad = true;
        goto end;
    }

    if (!(elf_file->section_names =
            read_section_data(elf_file, elf_file->header.e_shstrndx))) {
        bad = true;
        goto end;
    }

    if (read_symbol_tables(elf_file)) {
        bad = true;
        goto end;
    }

end:
    if (bad) {
        free_elf_file(elf_file);
        elf_file = NULL;
    }

    return elf_file;
}

static void
free_elf_file(struct elf_file *elf_file) {
    if (!elf_file)
        return;

    if (elf_file->fd >= 0) {
        close(elf_file->fd);
        elf_file->fd = -1;
    }

    free((void*)elf_file->section_names);
    for (Elf64_Half i = 0; i < elf_file->n_symbol_tables; ++i)
        free((void*)elf_file->symbol_tables[i].symbols);
    free(elf_file->symbol_tables);
    free(elf_file);
}

static bool
read_sections(struct elf_file *elf_file) {
    bool bad = false;
    size_t sections_size =
        sizeof(*elf_file->sections) * elf_file->header.e_shnum;

    if (!(elf_file->sections = malloc(sections_size))) {
        fprintf(stderr, "malloc(%lu): %d %s\n",
                sections_size, errno, strerror(errno));
        bad = true;
        goto end;
    }

    if (lseek(elf_file->fd, elf_file->header.e_shoff, SEEK_SET) < 0) {
        fprintf(stderr, "lseek(shoff): %d %s\n", errno, strerror(errno));
        bad = true;
        goto end;
    }

    if ((size_t)read(elf_file->fd, elf_file->sections, sections_size)
            != sections_size) {
        fprintf(stderr, "read(section_headers): %d %s\n",
                errno, strerror(errno));
        bad = true;
        goto end;
    }

end:
    if (bad) {
        free(elf_file->sections);
        elf_file->sections = NULL;
    }

    return bad;
}

static bool
read_symbol_tables(struct elf_file *elf_file __attribute__((unused))) {
    for (Elf64_Half i = SHN_BEGIN; i < elf_file->header.e_shnum; ++i) {
        if (elf_file->sections[i].sh_type == SHT_SYMTAB)
            ++elf_file->n_symbol_tables;
    }

    bool bad = false;
    size_t symbol_tables_size =
        sizeof(*elf_file->symbol_tables) * elf_file->n_symbol_tables;

    if (!(elf_file->symbol_tables = malloc(symbol_tables_size))) {
        fprintf(stderr, "malloc(%lu): %d %s",
                symbol_tables_size, errno, strerror(errno));
        bad = true;
        goto end;
    }

    for (Elf64_Half i = 0; i < elf_file->n_symbol_tables; ++i) {
        struct elf_symbol_table *symbol_table = &elf_file->symbol_tables[i];
        symbol_table->section = NULL;
        symbol_table->n_symbols = 0;
        symbol_table->symbols = NULL;
        symbol_table->names = NULL;
    }

    struct elf_symbol_table *symbol_table = &elf_file->symbol_tables[0];

    for (Elf64_Half i = SHN_BEGIN; i < elf_file->header.e_shnum; ++i) {
        const Elf64_Shdr *table_section = &elf_file->sections[i];
        if (table_section->sh_type != SHT_SYMTAB)
            continue;
        symbol_table->section = table_section;
        symbol_table->n_symbols =
            table_section->sh_size / sizeof(*symbol_table->symbols);

        if (!(symbol_table->symbols = read_section_data(elf_file, i))) {
            bad = true;
            goto end;
        }

        if (!(symbol_table->names =
                read_section_data(elf_file, table_section->sh_link))) {
            bad = true;
            goto end;
        }

        ++symbol_table;
    }

end:
    if (bad) {
        if (elf_file->symbol_tables) {
            for (Elf64_Half i = 0; i < elf_file->n_symbol_tables; ++i) {
                const struct elf_symbol_table *symbol_table =
                    &elf_file->symbol_tables[i];
                free((void*)symbol_table->symbols);
                free((void*)symbol_table->names);
            }

            free(elf_file->symbol_tables);
            elf_file->symbol_tables = NULL;
        }

        elf_file->n_symbol_tables = 0;
    }

    return bad;
}

static const void*
read_section_data(const struct elf_file *elf_file, Elf64_Half i) {
    const Elf64_Shdr *section = &elf_file->sections[i];
    bool bad = false;
    const void *data = NULL;

    if (!(data = malloc(section->sh_size))) {
        fprintf(stderr, "malloc(%lu): %d %s\n",
                section->sh_size, errno, strerror(errno));
        bad = true;
        goto end;
    }

    if (lseek(elf_file->fd, section->sh_offset, SEEK_SET) < 0) {
        fprintf(stderr, "lseek(%lu): %d %s\n",
                section->sh_offset, errno, strerror(errno));
        bad = true;
        goto end;
    }

    if ((size_t)read(elf_file->fd, (void*)data, section->sh_size)
            != section->sh_size) {
        fprintf(stderr, "read(sectio_names): %d %s\n", errno, strerror(errno));
        bad = true;
        goto end;
    }

end:
    if (bad) {
        free((void*)data);
        data = NULL;
    }

    return data;
}

static void print_elf_header(const struct elf_file*);
static void print_section_headers(const struct elf_file*);
static void print_symbol_tables(const struct elf_file*);

static void
print_elf_file(const struct elf_file *elf_file) {
    print_elf_header(elf_file);
    print_section_headers(elf_file);
    print_symbol_tables(elf_file);
}

static void
print_elf_header(const struct elf_file *elf_file) {
    const Elf64_Ehdr *header = &elf_file->header;
    puts("elf header:");
    printf(
        "\tmagic:\t\t%#x %c%c%c\n",
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

static void print_section_header(const struct elf_file*, Elf64_Half);

static void
print_section_headers(const struct elf_file *elf_file) {
    printf("\nsection headers:\n");
    printf("%-5s  %-16s %-18s %10s %10s %10s %18s %18s %9s %20s %10s %20s\n",
        "index", "name", "type", "link", "info", "flags", "virtual address",
        "file offset", "alignment", "size in file", "entry size",
        "num elements");

    for (Elf64_Half i = SHN_BEGIN; i < elf_file->header.e_shnum; ++i) {
        printf("%5u%c ", i, i == elf_file->header.e_shstrndx ? '*' : ' ');
        print_section_header(elf_file, i);
    }
}

static void print_section_type(Elf64_Word);

#define FLAG(flag) \
    (header->sh_flags & (flag) ? (#flag)[4] : ' ')

static void
print_section_header(const struct elf_file *elf_file, Elf64_Half i) {
    const Elf64_Shdr *header = &elf_file->sections[i];
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
    print_section_type(header->sh_type);
    printf("%10u %10u %s %#18lx %#18lx %9lu %20lu %10lu %20lu\n",
        header->sh_link, header->sh_info,
        flags,
        header->sh_addr, header->sh_offset,
        header->sh_addralign,
        header->sh_size, header->sh_entsize,
        header->sh_entsize ? header->sh_size / header->sh_entsize : 0);
}

static void
print_section_type(Elf64_Word type) {
    if (type <= SHT_NUM)
        printf("%-18s ", elf_section_type_str[type]);
    else if (SHT_LOOS <= type && type <= SHT_HIOS)
        printf("%-18s ", "OS-specific");
    else if (SHT_LOPROC <= type && type <= SHT_HIPROC)
        printf("%-18s ", "Processor-specific");
    else
        printf("%#-10x         ", type);
}

static void print_symbol_table(const struct elf_file*, Elf64_Half);

static void
print_symbol_tables(const struct elf_file *elf_file) {
    printf("\nsymbol tables:\n");
    printf("%20s  %-32s %-11s %-11s %-16s %18s %20s\n",
        "index", "name", "binding", "type", "section", "value", "size");
    for (Elf64_Half i = 0; i < elf_file->n_symbol_tables; ++i)
        print_symbol_table(elf_file, i);
}

static void print_symbol(const struct elf_file*, Elf64_Half, Elf64_Xword);

static void
print_symbol_table(const struct elf_file *elf_file, Elf64_Half i) {
    const struct elf_symbol_table *symbol_table = &elf_file->symbol_tables[i];
    printf("\nsymbol table: %s\n",
           &elf_file->section_names[symbol_table->section->sh_name]);
    for (Elf64_Xword j = 1; j < symbol_table->n_symbols; ++j)
        print_symbol(elf_file, i, j);
}

static void
print_symbol(const struct elf_file *elf_file, Elf64_Half i,
             Elf64_Xword j) {
    const struct elf_symbol_table *symbol_table = &elf_file->symbol_tables[i];
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

    Elf64_Half ndx = symbol->st_shndx;

    switch (ndx) {
    case SHN_ABS:
        printf("%-16s ", "ABS");
        break;
    case SHN_COMMON:
        printf("%-16s ", "COMMON");
        break;
    default:
        printf("%-16.16s ",
            &elf_file->section_names[elf_file->sections[ndx].sh_name]);
        break;
    }

    printf("%#18lx %20lu\n",
        symbol->st_value, symbol->st_size);
}
