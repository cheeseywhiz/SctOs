#include "readelf.h"
#include "elf.h"
#include "util.h"
#include "string.h"
#include <stdbool.h>

void
init_elf_file(struct elf_file *elf_file)
{
    elf_file->header = NULL;
    elf_file->program_headers = NULL;
    elf_file->sections = NULL;
    elf_file->section_names = NULL;
    elf_file->interpreter = NULL;
    elf_file->dynamic.dynamic = NULL;
    elf_file->dynamic.symbol_table = NULL;
    elf_file->n_symbol_tables = 0;
    elf_file->symbol_tables = NULL;
    elf_file->n_rel_tables = 0;
    elf_file->rel_tables = NULL;
}

static void* _elf_read(void *fd, Elf64_Off, Elf64_Xword);
/* returns if an error occurred */
static bool read_program_headers(struct elf_file*, void*);
static bool read_sections(struct elf_file*, void*);
static bool read_interpreter(struct elf_file*, void*);
static bool read_symbol_tables(struct elf_file*, void*);
static bool read_dynamic(struct elf_file*, void*);
static bool read_relocations(struct elf_file*, void*);
static const void* read_section_data(const struct elf_file*, void*, Elf64_Half);

bool
readelf(struct elf_file *elf_file, void *fd)
{
    if (!(elf_file->header = _elf_read(fd, 0, sizeof(*elf_file->header))))
        goto error;

    if (!ELF_VERIFY_MAGIC(*elf_file->header)) {
        elf_on_not_elf(elf_file);
        goto error;
    }

    if (read_program_headers(elf_file, fd))
        goto error;
    if (read_sections(elf_file, fd))
        goto error;
    if (!(elf_file->section_names =
            read_section_data(elf_file, fd, elf_file->header->e_shstrndx)))
        goto error;
    if (read_interpreter(elf_file, fd))
        goto error;
    if (read_symbol_tables(elf_file, fd))
        goto error;
    if (read_dynamic(elf_file, fd))
        goto error;
    if (read_relocations(elf_file, fd))
        goto error;
    return false;

error:
    free_elf_file(elf_file);
    init_elf_file(elf_file);
    return true;
}

void
free_elf_file(const struct elf_file *elf_file)
{
    if (!elf_file)
        return;

    elf_free(elf_file->header);
    elf_free(elf_file->program_headers);
    elf_free(elf_file->sections);
    elf_free(elf_file->section_names);
    elf_free(elf_file->interpreter);
    elf_free(elf_file->dynamic.dynamic);

    for (Elf64_Half i = 0; i < elf_file->n_symbol_tables; ++i) {
        const struct elf_symbol_table *symbol_table =
            &elf_file->symbol_tables[i];
        elf_free(symbol_table->symbols);
        elf_free(symbol_table->names);
    }

    elf_free(elf_file->symbol_tables);
    for (Elf64_Half i = 0; i < elf_file->n_rel_tables; ++i)
        elf_free(elf_file->rel_tables[i].relocations);
    elf_free(elf_file->rel_tables);
}

static bool
read_program_headers(struct elf_file *elf_file, void *fd)
{
    if (!elf_file->header->e_phoff)
        return false;
    return !(elf_file->program_headers = _elf_read(
        fd, elf_file->header->e_phoff,
        sizeof(*elf_file->program_headers) * elf_file->header->e_phnum));
}

static bool
read_sections(struct elf_file *elf_file, void *fd)
{
    return !(elf_file->sections = _elf_read(
        fd, elf_file->header->e_shoff,
        sizeof(*elf_file->sections) * elf_file->header->e_shnum));
}

static bool
read_interpreter(struct elf_file *elf_file, void *fd)
{
    for (Elf64_Half i = 1; i < elf_file->header->e_shnum; ++i) {
        const Elf64_Shdr *section = &elf_file->sections[i];
        const char *name = &elf_file->section_names[section->sh_name];
        if (strcmp(name, ".interp"))
            continue;
        return !(elf_file->interpreter = read_section_data(elf_file, fd, i));
    }

    return false;
}

static bool
read_symbol_tables(struct elf_file *elf_file, void *fd)
{
    for (Elf64_Half i = SHN_BEGIN; i < elf_file->header->e_shnum; ++i) {
        const Elf64_Shdr *section = &elf_file->sections[i];
        if (section->sh_type == SHT_SYMTAB || section->sh_type == SHT_DYNSYM)
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

    Elf64_Half table_i = 0;

    for (Elf64_Half i = SHN_BEGIN; i < elf_file->header->e_shnum; ++i) {
        const Elf64_Shdr *section = &elf_file->sections[i];
        if (section->sh_type != SHT_SYMTAB && section->sh_type != SHT_DYNSYM)
            continue;
        struct elf_symbol_table *symbol_table =
            &elf_file->symbol_tables[table_i++];
        symbol_table->section = section;
        symbol_table->n_symbols =
            section->sh_size / sizeof(*symbol_table->symbols);

        if (!(symbol_table->symbols = read_section_data(elf_file, fd, i)))
            goto error;
        if (!(symbol_table->names = read_section_data(
                elf_file, fd, (Elf64_Half)section->sh_link)))
            goto error;
        ++symbol_table;
    }

    return false;

error:
    if (elf_file->symbol_tables) {
        for (Elf64_Half i = 0; i < elf_file->n_symbol_tables; ++i) {
            const struct elf_symbol_table *symbol_table =
                &elf_file->symbol_tables[i];
            elf_free(symbol_table->symbols);
            elf_free(symbol_table->names);
        }

        elf_free(elf_file->symbol_tables);
        elf_file->symbol_tables = NULL;
    }

    elf_file->n_symbol_tables = 0;
    return true;
}

static bool
read_dynamic(struct elf_file *elf_file, void *fd)
{
    const Elf64_Shdr *dynamic_section = NULL;
    Elf64_Half dynamic_index = 0;

    for (Elf64_Half i = 1; i < elf_file->header->e_shnum; ++i) {
        const Elf64_Shdr *section = &elf_file->sections[i];
        if (section->sh_type != SHT_DYNAMIC)
            continue;
        dynamic_section = section;
        dynamic_index = i;
        break;
    }

    if (!dynamic_section)
        return false;
    if (!(elf_file->dynamic.dynamic =
            read_section_data(elf_file, fd, dynamic_index)))
        goto error;

    for (Elf64_Half i = 0; i < elf_file->n_symbol_tables; ++i) {
        const struct elf_symbol_table *symbol_table =
            &elf_file->symbol_tables[i];
        if (symbol_table->section->sh_type != SHT_DYNSYM)
            continue;
        elf_file->dynamic.symbol_table = symbol_table;
        break;
    }

    if (!elf_file->dynamic.symbol_table)
        goto error;
    return false;

error:
    elf_free(elf_file->dynamic.dynamic);
    elf_file->dynamic.symbol_table = NULL;
    return true;
}

static const void*
convert_rel_to_rela(const struct elf_file*, void*, Elf64_Half);

static bool
read_relocations(struct elf_file *elf_file, void *fd)
{
    for (Elf64_Half i = SHN_BEGIN; i < elf_file->header->e_shnum; ++i) {
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

    for (Elf64_Half i = SHN_BEGIN; i < elf_file->header->e_shnum; ++i) {
        const Elf64_Shdr *rel_section = &elf_file->sections[i];
        if (rel_section->sh_type != SHT_REL && rel_section->sh_type != SHT_RELA)
            continue;
        rel_table->section = rel_section;
        rel_table->n_relocations =
            rel_section->sh_size / rel_section->sh_entsize;

        if (rel_section->sh_type == SHT_RELA) {
            if (!(rel_table->relocations = read_section_data(elf_file, fd, i)))
                goto error;
        } else if (!(rel_table->relocations =
                         convert_rel_to_rela(elf_file, fd, i))) {
            goto error;
        }

        ++rel_table;
    }

    return false;

error:
    if (elf_file->rel_tables) {
        for (Elf64_Half i = 0; i < elf_file->n_rel_tables; ++i)
            elf_free(elf_file->rel_tables[i].relocations);
        elf_free(elf_file->rel_tables);
        elf_file->rel_tables = NULL;
    }

    elf_file->n_rel_tables = 0;
    return true;
}

static const void*
convert_rel_to_rela(const struct elf_file *elf_file, void *fd, Elf64_Half i)
{
    bool bad = false;
    Elf64_Rela *rela_table = NULL;
    const Elf64_Rel *rel_table;

    if (!(rel_table = read_section_data(elf_file, fd, i))) {
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
    elf_free(rel_table);
    if (!bad)
        return rela_table;
    elf_free(rela_table);
    return NULL;
}

static const void*
read_section_data(const struct elf_file *elf_file, void *fd, Elf64_Half i)
{
    const Elf64_Shdr *section = &elf_file->sections[i];
    return _elf_read(fd, section->sh_offset, section->sh_size);
}

/* wrap user's elf_read by passing it an allocated buffer */
static void*
_elf_read(void *fd, Elf64_Off offset, Elf64_Xword size)
{
    void *buf;
    if (!(buf = elf_alloc(size)))
        goto error;
    if (elf_read(fd, buf, offset, size))
        goto error;
    return buf;
error:
    elf_free(buf);
    return NULL;
}

/* noop defaults that indicate error
 * the user must implement these */

__weak bool
elf_read(void *fd __unused, void *buf __unused, Elf64_Off offset __unused,
         Elf64_Xword size __unused)
{
    return true;
}

__weak void*
elf_alloc(Elf64_Xword size __unused)
{
    return NULL;
}

__weak void
elf_free(const void *ptr __unused)
{
}

/* the user may define this function */

__weak void
elf_on_not_elf(const struct elf_file *elf_file __unused)
{
}
