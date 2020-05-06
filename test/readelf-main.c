/* TODO: port this program to my operating system */
#include "readelf.h"
#include "test-readelf.h"
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

static bool file_header_flag = false;
static bool program_headers_flag = false;
static bool section_headers_flag = false;
static bool dynamic_flag = false;
static bool symbols_flag = false;
static bool relocs_flag = false;
static bool mmap_flag = false;
static const char *const usage =
/* Usage: readelf */ "(-|+)[flSdsrma]... [--] file...\n"
"\n"
"\t-f file header\n"
"\t-l program headers\n"
"\t-S section headers\n"
"\t-d dynamic segment\n"
"\t-s symbol tables\n"
"\t-r relocations\n"
"\t-m memory map\n"
"\t-a all the above\n";

enum set_flags_exit { SF_CONTINUE, SF_EXIT_0, SF_EXIT_1 };
static enum set_flags_exit set_flags(const char **argv[]);
static void print_elf_file(const struct elf_file*);

int
main(int argc __unused, const char *argv[])
{
    switch (set_flags(&argv)) {
    case SF_CONTINUE:
        break;
    case SF_EXIT_0:
        return 0;
    case SF_EXIT_1:
        return 1;
    }

    fprintf(stderr, "sizeof(Elf64_Ehdr): %lu\n", sizeof(Elf64_Ehdr));
    fprintf(stderr, "sizeof(Elf64_Phdr): %lu\n", sizeof(Elf64_Phdr));
    fprintf(stderr, "sizeof(Elf64_Shdr): %lu\n", sizeof(Elf64_Shdr));
    fprintf(stderr, "sizeof(Elf64_Sym):  %lu\n", sizeof(Elf64_Sym));
    fprintf(stderr, "sizeof(Elf64_Rel):  %lu\n", sizeof(Elf64_Rel));
    fprintf(stderr, "sizeof(Elf64_Rela): %lu\n", sizeof(Elf64_Rela));
    fprintf(stderr, "sizeof(Elf64_Dyn):  %lu\n", sizeof(Elf64_Dyn));
    int returncode = 0;
    const char *fname;

    while ((fname = *argv++)) {
        puts("");
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

static enum set_flags_exit set_flag(char direction, char flag);
static void print_help(void);
static const char *program_name;

static enum set_flags_exit
set_flags(const char **argv[])
{
    program_name = **argv;
    enum set_flags_exit exit = SF_CONTINUE;
    const char *arg;

    while ((arg = *(++*argv))) {
        char direction = arg[0];

        if (direction == '-') {
            if (arg[1] == '-' && !arg[2]) {
                ++*argv;
                goto end;
            }
        } else if (direction != '+') {
            goto end;
        }

        if (!arg[1])
            goto end;
        char flag;

        while ((flag = *++arg)) {
            if ((exit = set_flag(direction, flag)) != SF_CONTINUE)
                goto end;
        }
    }

end:
    if (exit != SF_CONTINUE)
        return exit;
    bool no_flags = !file_header_flag && !file_header_flag
        && !program_headers_flag && !section_headers_flag && !dynamic_flag
        && !symbols_flag && !relocs_flag && !mmap_flag;
    bool no_files = !**argv;

    if (no_flags && no_files) {
        print_help();
        return SF_EXIT_0;
    } else if (no_flags && !no_files) {
        printf("no output flags specified\n");
        print_help();
        return SF_EXIT_1;
    } else if (!no_flags && no_files) {
        printf("no input files specified\n");
        print_help();
        return SF_EXIT_1;
    } else {
        return SF_CONTINUE;
    }
}

static enum set_flags_exit
set_flag(char direction, char flag)
{
    bool setting = direction == '-';

    switch (flag) {
    case 'f':
        file_header_flag = setting;
        break;
    case 'l':
        program_headers_flag = setting;
        break;
    case 'S':
        section_headers_flag = setting;
        break;
    case 'd':
        dynamic_flag = setting;
        break;
    case 's':
        symbols_flag = setting;
        break;
    case 'r':
        relocs_flag = setting;
        break;
    case 'm':
        mmap_flag = setting;
        break;
    case 'a':
        file_header_flag = setting;
        program_headers_flag = setting;
        section_headers_flag = setting;
        dynamic_flag = setting;
        symbols_flag = setting;
        relocs_flag = setting;
        mmap_flag = setting;
        break;
    case 'h':
        print_help();
        return SF_EXIT_0;
    default:
        printf("unknown flag: %c\n", flag);
        print_help();
        return SF_EXIT_1;
    }

    return SF_CONTINUE;
}

static void
print_help(void)
{
    printf("Usage: %s %s", program_name, usage);
}

static void
print_elf_file(const struct elf_file *elf_file)
{
    if (file_header_flag)
        print_elf_header(elf_file->header);
    if (program_headers_flag)
        print_program_headers(elf_file);
    if (section_headers_flag)
        print_section_headers(elf_file);
    if (dynamic_flag)
        print_dynamic(elf_file);
    if (symbols_flag)
        print_symbol_tables(elf_file);
    if (relocs_flag)
        print_relocations(elf_file);
    if ((elf_file->header->e_type != ET_EXEC
         && elf_file->header->e_type != ET_DYN)
        || !mmap_flag)
        return;
    Elf64_Xword n_entries;
    struct mmap_entry *entries;
    if (!(entries = mmap_get(elf_file, &n_entries)))
        return;
    mmap_sort(entries, n_entries);
    mmap_print(entries, n_entries);
    mmap_free(entries, n_entries);
}
