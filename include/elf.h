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

#define _ENUM_STR(value) [value] = #value

/* elf64 table 3 */
enum elf_class {
    ELFCLASSNONE,
    ELFCLASS32,
    ELFCLASS64,
};

static const char *const elf_class_str[] = {
    _ENUM_STR(ELFCLASSNONE),
    _ENUM_STR(ELFCLASS32),
    _ENUM_STR(ELFCLASS64),
};

/* elf64 table 4 */
enum elf_endianness {
    EE_NONE,
    EE_LITTLE,
    EE_BIG,
};

static const char *const elf_endianness_str[] = {
    _ENUM_STR(EE_NONE),
    _ENUM_STR(EE_LITTLE),
    _ENUM_STR(EE_BIG),
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
    _ENUM_STR(ELFOSABI_SYSV),
    _ENUM_STR(ELFOSABI_HPUX),
    _ENUM_STR(ELFOSABI_NETBSD),
    _ENUM_STR(ELFOSABI_GNU),
    _ENUM_STR(ELFOSABI_SOLARIS),
    _ENUM_STR(ELFOSABI_AIX),
    _ENUM_STR(ELFOSABI_IRIX),
    _ENUM_STR(ELFOSABI_FREEBSD),
    _ENUM_STR(ELFOSABI_TRU64),
    _ENUM_STR(ELFOSABI_MODESTO),
    _ENUM_STR(ELFOSABI_OPENBSD),
    _ENUM_STR(ELFOSABI_ARM_AEABI),
    _ENUM_STR(ELFOSABI_ARM),
    _ENUM_STR(ELFOSABI_STANDALONE),
};

/* elf64 table 6 */
enum elf_type {
    ET_NONE,
    ET_REL,
    ET_EXEC,
    ET_DYN,
    ET_CORE,
    ET_LOOS = 0xfe00,
    ET_HIOS = 0xfeff,
    ET_LOPROC = 0xff00,
    ET_HIPROC = 0xffff,
};

static const char *const elf_type_str[] = {
    _ENUM_STR(ET_NONE),
    _ENUM_STR(ET_REL),
    _ENUM_STR(ET_EXEC),
    _ENUM_STR(ET_DYN),
    _ENUM_STR(ET_CORE),
    _ENUM_STR(ET_LOOS),
    _ENUM_STR(ET_HIOS),
    _ENUM_STR(ET_LOPROC),
    _ENUM_STR(ET_HIPROC),
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
    _ENUM_STR(EM_NONE),
    _ENUM_STR(EM_M32),
    _ENUM_STR(EM_SPARC),
    _ENUM_STR(EM_386),
    _ENUM_STR(EM_68K),
    _ENUM_STR(EM_88K),
    _ENUM_STR(EM_860),
    _ENUM_STR(EM_MIPS),
    _ENUM_STR(EM_MIPS_RS4_BE),
    _ENUM_STR(EM_X86_64),
};

enum elf_version {
    EV_NONE,
    EV_CURRENT,
};

static const char *const elf_version_str[] = {
    _ENUM_STR(EV_NONE),
    _ENUM_STR(EV_CURRENT),
};

/* elf64 table 7 */
enum elf_special_section_index {
    SHN_UNDEF,
    SHN_BEGIN,
    SHN_LOPROC = 0xff00,
    SHN_HIPROC = 0xff1f,
    SHN_LOOS = 0xff20,
    SHN_HIOS = 0xff3f,
    SHN_ABS = 0xfff1,
    SHN_COMMON = 0xfff2,
};

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
#define SHT_NUM SHT_GROUP
    SHT_LOOS = 0x60000000,
    SHT_HIOS = 0x6fffffff,
    SHT_LOPROC = 0x70000000,
    SHT_HIPROC = 0x7fffffff,
};

static const char *const elf_section_type_str[] = {
    _ENUM_STR(SHT_NULL),
    _ENUM_STR(SHT_PROGBITS),
    _ENUM_STR(SHT_SYMTAB),
    _ENUM_STR(SHT_STRTAB),
    _ENUM_STR(SHT_RELA),
    _ENUM_STR(SHT_HASH),
    _ENUM_STR(SHT_DYNAMIC),
    _ENUM_STR(SHT_NOTE),
    _ENUM_STR(SHT_NOBITS),
    _ENUM_STR(SHT_REL),
    _ENUM_STR(SHT_RESERVED),
    _ENUM_STR(SHT_DYNSYM),
    _ENUM_STR(SHT_INIT_ARRAY),
    _ENUM_STR(SHT_FINI_ARRAY),
    _ENUM_STR(SHT_PREINIT_ARRAY),
    _ENUM_STR(SHT_GROUP),
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

struct elf_file {
    const char *fname;
    Elf64_Ehdr  header;
    Elf64_Shdr *sections;
    const char *section_names;
};
