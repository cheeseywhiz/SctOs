#pragma once
#include <stdint.h>
#include <stddef.h>

/* elf64 table 1 */
typedef uint64_t Elf64_Addr;
typedef uint64_t Elf64_Off;
typedef uint16_t Elf64_Half;
typedef uint32_t Elf64_Word;
typedef  int32_t Elf64_Sword;
typedef uint64_t Elf64_Xword;
typedef  int64_t Elf64_Sxword;

/* elf64 table 2 */
enum elf_ident_index {
    EI_MAG0,
    EI_MAG1,
    EI_MAG2,
    EI_MAG3,
    EI_CLASS,
    EI_ENDIANNESS,
    EI_VERSION,
    EI_OSABI,
    EI_ABIVERSION,
    EI_PAD,
    EI_NIDENT = 16,
};

/* elf64 figure 2 */
typedef struct {
    unsigned char e_ident[EI_NIDENT];
    Elf64_Half    e_type;
    Elf64_Half    e_machine;
    Elf64_Word    e_version;
    Elf64_Addr    e_entry;
    Elf64_Off     e_phoff;
    Elf64_Off     e_shoff;
    Elf64_Word    e_flags;
    Elf64_Half    e_ehsize;
    Elf64_Half    e_phentsize;
    Elf64_Half    e_phnum;
    Elf64_Half    e_shentsize;
    Elf64_Half    e_shnum;
    Elf64_Half    e_shstrndx;
} Elf64_Ehdr;

#define ELF_VERIFY_MAGIC(hdr) \
    ((hdr).e_ident[EI_MAG0] == 0x7f && \
     (hdr).e_ident[EI_MAG1] == 'E' && \
     (hdr).e_ident[EI_MAG2] == 'L' && \
     (hdr).e_ident[EI_MAG3] == 'F')

#define _ENUM_STR(prefix, suffix) [prefix ## suffix] = (#suffix)

/* elf64 table 3 */
enum elf_class {
    ELFCLASSNONE,
    ELFCLASS32,
    ELFCLASS64,
};

static const char *const elf_class_str[] = {
    _ENUM_STR(ELFCLASS, NONE),
    _ENUM_STR(ELFCLASS, 32),
    _ENUM_STR(ELFCLASS, 64),
};

/* elf64 table 4 */
enum elf_endianness {
    EE_NONE,
    EE_LITTLE,
    EE_BIG,
};

static const char *const elf_endianness_str[] = {
    _ENUM_STR(EE_, NONE),
    _ENUM_STR(EE_, LITTLE),
    _ENUM_STR(EE_, BIG),
};

/* elf64 table 5 + glibc elf/elf.h */
enum elf_osabi {
    ELFOSABI_SYSV,
    ELFOSABI_HPUX,
    ELFOSABI_NETBSD,
    ELFOSABI_GNU,
#define ELFOSABI_LINUX ELFOSABI_GNU
    ELFOSABI_SOLARIS,
    ELFOSABI_AIX,
    ELFOSABI_IRIX,
    ELFOSABI_FREEBSD,
    ELFOSABI_TRU64,
    ELFOSABI_MODESTO,
    ELFOSABI_OPENBSD,
    ELFOSABI_ARM_AEABI,
    ELFOSABI_ARM,
    ELFOSABI_STANDALONE = 255,
};

static const char *const elf_osabi_str[] = {
    _ENUM_STR(ELFOSABI_, SYSV),
    _ENUM_STR(ELFOSABI_, HPUX),
    _ENUM_STR(ELFOSABI_, NETBSD),
    _ENUM_STR(ELFOSABI_, GNU),
    _ENUM_STR(ELFOSABI_, SOLARIS),
    _ENUM_STR(ELFOSABI_, AIX),
    _ENUM_STR(ELFOSABI_, IRIX),
    _ENUM_STR(ELFOSABI_, FREEBSD),
    _ENUM_STR(ELFOSABI_, TRU64),
    _ENUM_STR(ELFOSABI_, MODESTO),
    _ENUM_STR(ELFOSABI_, OPENBSD),
    _ENUM_STR(ELFOSABI_, ARM_AEABI),
    _ENUM_STR(ELFOSABI_, ARM),
    _ENUM_STR(ELFOSABI_, STANDALONE),
};

/* elf64 table 6 */
enum elf_type {
    ET_NONE,
    ET_REL,
    ET_EXEC,
    ET_DYN,
    ET_CORE,
#define ET_LOOS 0xfe00
#define ET_HIOS 0xfeff
#define ET_LOPROC 0xff00
#define ET_HIPROC 0xffff
};

static const char *const elf_type_str[] = {
    _ENUM_STR(ET_, NONE),
    _ENUM_STR(ET_, REL),
    _ENUM_STR(ET_, EXEC),
    _ENUM_STR(ET_, DYN),
    _ENUM_STR(ET_, CORE),
};

enum elf_machine {
    EM_NONE,
    EM_M32,
    EM_SPARC,
    EM_386,
    EM_68K,
    EM_88K,
    EM_860,
    EM_MIPS,
    EM_MIPS_RS4_BE,
    EM_X86_64 = 62,
};

static const char *const elf_machine_str[] = {
    _ENUM_STR(EM_, NONE),
    _ENUM_STR(EM_, M32),
    _ENUM_STR(EM_, SPARC),
    _ENUM_STR(EM_, 386),
    _ENUM_STR(EM_, 68K),
    _ENUM_STR(EM_, 88K),
    _ENUM_STR(EM_, 860),
    _ENUM_STR(EM_, MIPS),
    _ENUM_STR(EM_, MIPS_RS4_BE),
    _ENUM_STR(EM_, X86_64),
};

enum elf_version {
    EV_NONE,
    EV_CURRENT,
};

static const char *const elf_version_str[] = {
    _ENUM_STR(EV_, NONE),
    _ENUM_STR(EV_, CURRENT),
};

/* elf64 table 7 */
#define SHN_UNDEF 0
#define SHN_BEGIN 1
#define SHN_LOPROC 0xff00
#define SHN_HIPROC 0xff1f
#define SHN_LOOS 0xff20
#define SHN_HIOS 0xff3f
#define SHN_ABS 0xfff1
#define SHN_COMMON 0xfff2

/* elf64 figure 3 */
typedef struct {
    Elf64_Word  sh_name;
    Elf64_Word  sh_type;
    Elf64_Xword sh_flags;
    Elf64_Addr  sh_addr;
    Elf64_Off   sh_offset;
    Elf64_Xword sh_size;
    Elf64_Word  sh_link;
    Elf64_Word  sh_info;
    Elf64_Xword sh_addralign;
    Elf64_Xword sh_entsize;
} Elf64_Shdr;

/* elf64 table 8 + glibc elf/elf.h */
enum elf_section_type {
    SHT_NULL,
    SHT_PROGBITS,
    SHT_SYMTAB,
    SHT_STRTAB,
    SHT_RELA,
    SHT_HASH,
    SHT_DYNAMIC,
    SHT_NOTE,
    SHT_NOBITS,
    SHT_REL,
    SHT_RESERVED,
    SHT_DYNSYM,
    SHT_INIT_ARRAY,
    SHT_FINI_ARRAY,
    SHT_PREINIT_ARRAY,
    SHT_GROUP,
/* number of defined types */
    SHT_NUM,
#define SHT_LOOS 0x60000000
#define SHT_HIOS 0x6fffffff
#define SHT_LOPROC 0x70000000
#define SHT_HIPROC 0x7fffffff
};

static const char *const elf_section_type_str[] = {
    _ENUM_STR(SHT_, NULL),
    _ENUM_STR(SHT_, PROGBITS),
    _ENUM_STR(SHT_, SYMTAB),
    _ENUM_STR(SHT_, STRTAB),
    _ENUM_STR(SHT_, RELA),
    _ENUM_STR(SHT_, HASH),
    _ENUM_STR(SHT_, DYNAMIC),
    _ENUM_STR(SHT_, NOTE),
    _ENUM_STR(SHT_, NOBITS),
    _ENUM_STR(SHT_, REL),
    _ENUM_STR(SHT_, RESERVED),
    _ENUM_STR(SHT_, DYNSYM),
    _ENUM_STR(SHT_, INIT_ARRAY),
    _ENUM_STR(SHT_, FINI_ARRAY),
    _ENUM_STR(SHT_, PREINIT_ARRAY),
    _ENUM_STR(SHT_, GROUP),
};

/* elf64 table 9 + glibc elf/elf.h */
enum elf_section_flags {
    SHF_WRITE = 1 << 0,            /* writable */
    SHF_ALLOC = 1 << 1,            /* occupies memory in execution model */
    SHF_EXECINSTR = 1 << 2,        /* executable */
    SHF_MERGE = 1 << 3,            /* might be merged */
    SHF_STRINGS = 1 << 5,          /* Contains null-terminated strings */
    SHF_INFO_LINK = 1 << 6,        /* sh_info contains SHT index */
    SHF_LINK_ORDER = 1 << 7,       /* preserve order after combining */
    SHF_OS_NONCONFORMING = 1 << 8, /* non-standard handling required */
    SHF_GROUP = 1 << 9,            /* section is member of a group */
    SHF_TLS = 1 << 10,             /* thread local storage */
    SHF_COMPRESSED = 1 << 11,      /* contains compressed data */
    SHF_MASKOS = 0xff00000,
#define SHF_MASKPROC 0xf0000000
};

static const char *const elf_section_flags_str[] = {
    "COMPRESSED", "TLS", "OS_NONCONFORMING", "LINK_ORDER", "INFO_LINK",
    "STRINGS", "MERGE", "EXECINSTR", "ALLOC", "WRITE",
};

/* elf64 figure 4 */
typedef struct {
    Elf64_Word    st_name;  /* symbol name offset in associated string table */
    unsigned char st_info;  /* type & binding, see tables 14 & 15 */
    unsigned char st_other; /* reserved */
    Elf64_Half    st_shndx; /* section table index */
    Elf64_Addr    st_value; /* virtual address for exececutables and shared
                             * object files */
    Elf64_Xword   st_size;  /* size of object, if known */
} Elf64_Sym;

#define ELF_SYMBOL_BINDING(sym) ((sym).st_info >> 4)
#define ELF_SYMBOL_TYPE(sym) ((sym).st_info & 0xf)

/* elf64 table 14 */
enum elf_symbol_binding {
    STB_LOCAL,
    STB_GLOBAL,
    STB_WEAK,
    STB_NUM,
#define STB_LOOS 10
#define STB_HIOS 12
#define STB_LOPROC 13
#define STB_HIPROC 15
};

static const char *const elf_symbol_binding_str[] = {
    _ENUM_STR(STB_, LOCAL),
    _ENUM_STR(STB_, GLOBAL),
    _ENUM_STR(STB_, WEAK),
};

/* elf64 table 15 */
enum elf_symbol_type {
    STT_NOTYPE,
    STT_OBJECT,
    STT_FUNC,
    STT_SECTION,
    STT_FILE,
/* glibc elf/elf.h */
    STT_COMMON,
    STT_TLS,
    STT_NUM,
#define STT_LOOS 10
#define STT_HIOS 12
#define STT_LOPROC 13
#define STT_HIPROC 15
};

static const char *const elf_symbol_type_str[] = {
    _ENUM_STR(STT_, NOTYPE),
    _ENUM_STR(STT_, OBJECT),
    _ENUM_STR(STT_, FUNC),
    _ENUM_STR(STT_, SECTION),
    _ENUM_STR(STT_, FILE),
    _ENUM_STR(STT_, COMMON),
    _ENUM_STR(STT_, TLS),
};

struct elf_symbol_table;
struct elf_rel_table;
struct elf_rela_table;

struct elf_file {
    Elf64_Ehdr               header;
    const char              *fname;
    int                      fd;
    Elf64_Shdr              *sections;
    const char              *section_names;
    Elf64_Half               n_symbol_tables;
    struct elf_symbol_table *symbol_tables;
};

struct elf_symbol_table {
    const Elf64_Shdr *section;
    Elf64_Xword       n_symbols;
    const Elf64_Sym  *symbols;
    const char       *names;
};
