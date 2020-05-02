#include "readelf.h"
#include "elf.h"
#include "string.h"
#include <stdbool.h>

/* returns if an error occurred */
static bool read_program_headers(struct elf_file*);
static bool read_sections(struct elf_file*);
static bool read_interpreter(struct elf_file*);
static bool read_symbol_tables(struct elf_file*);
static bool read_relocations(struct elf_file*);
static const void* read_section_data(const struct elf_file*, Elf64_Half);

const struct elf_file*
readelf(const char *fname)
{
    struct elf_file *elf_file = NULL;

    if (!(elf_file = elf_alloc(sizeof(*elf_file))))
        goto error;

    elf_file->fd = NULL;
    elf_file->fname = fname;
    elf_file->program_headers = NULL;
    elf_file->sections = NULL;
    elf_file->section_names = NULL;
    elf_file->interpreter = NULL;
    elf_file->n_symbol_tables = 0;
    elf_file->symbol_tables = NULL;
    elf_file->n_rel_tables = 0;
    elf_file->rel_tables = NULL;

    if (elf_open(elf_file))
        goto error;
    if (!elf_read(elf_file, &elf_file->header, 0, sizeof(elf_file->header)))
        goto error;

    if (!ELF_VERIFY_MAGIC(elf_file->header)) {
        elf_on_not_elf(elf_file);
        goto error;
    }

    if (read_program_headers(elf_file))
        goto error;
    if (read_sections(elf_file))
        goto error;
    if (!(elf_file->section_names =
            read_section_data(elf_file, elf_file->header.e_shstrndx)))
        goto error;
    if (read_interpreter(elf_file))
        goto error;
    if (read_symbol_tables(elf_file))
        goto error;
    if (read_relocations(elf_file))
        goto error;
    elf_close(elf_file);
    return elf_file;

error:
    elf_close(elf_file);
    free_elf_file(elf_file);
    return NULL;
}

void
free_elf_file(const struct elf_file *elf_file)
{
    if (!elf_file)
        return;

    ELF_FREE(elf_file->program_headers);
    ELF_FREE(elf_file->section_names);
    ELF_FREE(elf_file->interpreter);

    for (Elf64_Half i = 0; i < elf_file->n_symbol_tables; ++i) {
        const struct elf_symbol_table *symbol_table =
            &elf_file->symbol_tables[i];
        ELF_FREE(symbol_table->symbols);
        ELF_FREE(symbol_table->names);
    }

    ELF_FREE(elf_file->symbol_tables);
    for (Elf64_Half i = 0; i < elf_file->n_rel_tables; ++i)
        ELF_FREE(elf_file->rel_tables[i].relocations);
    ELF_FREE(elf_file);
}

static bool
read_program_headers(struct elf_file *elf_file)
{
    if (!elf_file->header.e_phoff)
        return false;
    return !(elf_file->program_headers = elf_read(
        elf_file, NULL, elf_file->header.e_phoff,
        sizeof(*elf_file->program_headers) * elf_file->header.e_phnum));
}

static bool
read_sections(struct elf_file *elf_file)
{
    return !(elf_file->sections = elf_read(
        elf_file, NULL, elf_file->header.e_shoff,
        sizeof(*elf_file->sections) * elf_file->header.e_shnum));
}

static bool
read_interpreter(struct elf_file *elf_file)
{
    for (Elf64_Half i = 1; i < elf_file->header.e_shnum; ++i) {
        const Elf64_Shdr *section = &elf_file->sections[i];
        const char *name = &elf_file->section_names[section->sh_name];
        if (strcmp(name, ".interp"))
            continue;
        return !(elf_file->interpreter = read_section_data(elf_file, i));
    }

    return false;
}

static bool
read_symbol_tables(struct elf_file *elf_file)
{
    for (Elf64_Half i = SHN_BEGIN; i < elf_file->header.e_shnum; ++i) {
        if (elf_file->sections[i].sh_type == SHT_SYMTAB)
            ++elf_file->n_symbol_tables;
    }

    Elf64_Xword symbol_tables_size =
        sizeof(*elf_file->symbol_tables) * elf_file->n_symbol_tables;

    if (!(elf_file->symbol_tables = elf_alloc(symbol_tables_size)))
        goto error;

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

        if (!(symbol_table->symbols = read_section_data(elf_file, i)))
            goto error;
        if (!(symbol_table->names = read_section_data(
                elf_file, (Elf64_Half)table_section->sh_link)))
            goto error;
        ++symbol_table;
    }

    return false;

error:
    if (elf_file->symbol_tables) {
        for (Elf64_Half i = 0; i < elf_file->n_symbol_tables; ++i) {
            symbol_table = &elf_file->symbol_tables[i];
            ELF_FREE(symbol_table->symbols);
            ELF_FREE(symbol_table->names);
        }

        ELF_FREE(elf_file->symbol_tables);
        elf_file->symbol_tables = NULL;
    }

    elf_file->n_symbol_tables = 0;
    return true;
}

static const void*
convert_rel_to_rela(const struct elf_file*, Elf64_Half);

static bool
read_relocations(struct elf_file *elf_file)
{
    for (Elf64_Half i = SHN_BEGIN; i < elf_file->header.e_shnum; ++i) {
        Elf64_Word type = elf_file->sections[i].sh_type;
        if (type == SHT_REL || type == SHT_RELA)
            ++elf_file->n_rel_tables;
    }

    Elf64_Xword rel_tables_size =
        sizeof(*elf_file->rel_tables) * elf_file->n_rel_tables;

    if (!(elf_file->rel_tables = elf_alloc(rel_tables_size)))
        goto error;

    for (Elf64_Half i = 0; i < elf_file->n_rel_tables; ++i) {
        struct elf_rel_table *rel_table = &elf_file->rel_tables[i];
        rel_table->section = NULL;
        rel_table->n_relocations = 0;
        rel_table->relocations = NULL;
    }

    struct elf_rel_table *rel_table = &elf_file->rel_tables[0];

    for (Elf64_Half i = SHN_BEGIN; i < elf_file->header.e_shnum; ++i) {
        const Elf64_Shdr *rel_section = &elf_file->sections[i];
        if (rel_section->sh_type != SHT_REL && rel_section->sh_type != SHT_RELA)
            continue;
        rel_table->section = rel_section;
        rel_table->n_relocations =
            rel_section->sh_size / rel_section->sh_entsize;

        if (rel_section->sh_type == SHT_RELA) {
            if (!(rel_table->relocations = read_section_data(elf_file, i)))
                goto error;
        } else if (!(rel_table->relocations =
                         convert_rel_to_rela(elf_file, i))) {
            goto error;
        }

        ++rel_table;
    }

    return false;

error:
    if (elf_file->rel_tables) {
        for (Elf64_Half i = 0; i < elf_file->n_rel_tables; ++i)
            ELF_FREE(elf_file->rel_tables[i].relocations);
        ELF_FREE(elf_file->rel_tables);
        elf_file->rel_tables = NULL;
    }

    elf_file->n_rel_tables = 0;
    return true;
}

static const void*
convert_rel_to_rela(const struct elf_file *elf_file, Elf64_Half i)
{
    bool bad = false;
    Elf64_Rela *rela_table = NULL;
    const Elf64_Rel *rel_table;

    if (!(rel_table = read_section_data(elf_file, i))) {
        bad = true;
        goto end;
    }

    const Elf64_Shdr *rel_section = &elf_file->sections[i];
    Elf64_Xword n_elems = rel_section->sh_size / sizeof(*rel_section);
    Elf64_Xword rela_size = sizeof(*rela_table) * n_elems;

    if (!(rela_table = elf_alloc(rela_size))) {
        bad = true;
        goto end;
    }

    for (Elf64_Xword j = 0; j < n_elems; ++j) {
        const Elf64_Rel *rel = &rel_table[j];
        Elf64_Rela *rela = &rela_table[j];
        rela->r_offset = rel->r_offset;
        rela->r_info = rel->r_info;
        rela->r_addend = 0;
    }

end:
    ELF_FREE(rel_table);
    if (!bad)
        return rela_table;
    ELF_FREE(rela_table);
    return NULL;
}

static const void*
read_section_data(const struct elf_file *elf_file, Elf64_Half i)
{
    const Elf64_Shdr *section = &elf_file->sections[i];
    return elf_read(elf_file, NULL, section->sh_offset, section->sh_size);
}
