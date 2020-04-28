/* TODO: port this program to my operating system */
#include "elf.h"
#include "util.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>

/* returns if an error occurred */
static bool readelf(struct elf_file*, const char*);
static void print_elf_file(const struct elf_file*);

int
main(int argc, char *argv[]) {
    printf("sizeof(Elf64_Ehdr): %lu\n", sizeof(Elf64_Ehdr));
    printf("sizeof(Elf64_Shdr): %lu\n", sizeof(Elf64_Shdr));
    printf("flags: ");
    for (size_t i = 0; i < ARRAY_LENGTH(elf_section_flags_str); ++i)
        printf("%s ", elf_section_flags_str[i]);
    puts("");
    int returncode = 0;

    for (int i = 1; i < argc; ++i) {
        puts("");
        struct elf_file elf_file;
        const char *fname = argv[i];
        printf("file:\t\t\t%s\n", fname);

        if (readelf(&elf_file, fname)) {
            returncode = 1;
            continue;
        }

        print_elf_file(&elf_file);
    }

    return returncode;
}

/* returns if an error occurred */
static bool read_sections(struct elf_file*, int);
static bool read_section_names(struct elf_file*, int);

static bool
readelf(struct elf_file *elf_file, const char *fname) {
    elf_file->fname = fname;
    elf_file->sections = NULL;
    elf_file->section_names = NULL;
    bool bad = false;
    int fd;

    if ((fd = open(elf_file->fname, O_RDONLY)) < 0) {
        fprintf(stderr, "open(\"%s\");\n", elf_file->fname);
        bad = true;
        goto end;
    }

    if (read(fd, &elf_file->header, sizeof(elf_file->header))
            < (ssize_t)sizeof(elf_file->header)) {
        fprintf(stderr, "read(elf_header)\n");
        bad = true;
        goto end;
    }

    if (!ELF_VERIFY_MAGIC(elf_file->header)) {
        fprintf(stderr, "file is not ELF\n");
        bad = true;
        goto end;
    }

    if (read_sections(elf_file, fd)) {
        bad = true;
        goto end;
    }

    if (read_section_names(elf_file, fd)) {
        bad = true;
        goto end;
    }

end:
    close(fd);
    return bad;
}

static bool
read_sections(struct elf_file *elf_file, int fd) {
    bool bad = false;
    size_t sections_size = sizeof(*elf_file->sections) * elf_file->header.e_shnum;

    if (!(elf_file->sections = malloc(sections_size))) {
        fprintf(stderr, "malloc(%lu);\n", sections_size);
        bad = true;
        goto end;
    }

    if (lseek(fd, elf_file->header.e_shoff, SEEK_SET) < 0) {
        fprintf(stderr, "lseek(shoff)\n");
        bad = true;
        goto end;
    }

    if (read(fd, elf_file->sections, sections_size) < (ssize_t)sections_size) {
        fprintf(stderr, "read(section_headers)\n");
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
read_section_names(struct elf_file *elf_file, int fd) {
    const Elf64_Shdr *section = &elf_file->sections[elf_file->header.e_shstrndx];
    bool bad = false;

    if (!(elf_file->section_names = malloc(section->sh_size))) {
        fprintf(stderr, "malloc(%lu)\n", section->sh_size);
        bad = true;
        goto end;
    }

    if (lseek(fd, section->sh_offset, SEEK_SET) < 0) {
        fprintf(stderr, "lseek(%lu)\n", section->sh_offset);
        bad = true;
        goto end;
    }

    if (read(fd, (char*)elf_file->section_names, section->sh_size)
            < (ssize_t)section->sh_size) {
        fprintf(stderr, "read(sectio_names)\n");
        bad = true;
        goto end;
    }

end:
    if (bad) {
        free((void*)elf_file->section_names);
        elf_file->section_names = NULL;
    }

    return bad;
}

static void print_elf_header(const struct elf_file*);
static void print_section_headers(const struct elf_file*);

static void
print_elf_file(const struct elf_file *elf_file) {
    print_elf_header(elf_file);
    print_section_headers(elf_file);
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
