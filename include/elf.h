#pragma once
#include <stdint.h>
#include <stddef.h>
#include "util.h"

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
    EI_MAG0,        /* ELF_VERIFY_MAGIC */
    EI_MAG1,        /*  ^ */
    EI_MAG2,        /*  | */
    EI_MAG3,        /* -| */
    EI_CLASS,       /* enum elf_class */
    EI_ENDIANNESS,  /* enum elf_endianness */
    EI_VERSION,     /* enum elf_version */
    EI_OSABI,       /* enum elf_osabi */
    EI_ABIVERSION,  /* no enum rn */
    EI_PAD,
    EI_NIDENT = 16, 
};

/* elf64 figure 2 */
typedef struct {
    unsigned char e_ident[EI_NIDENT];
    Elf64_Half    e_type;      /* enum elf_type */
    Elf64_Half    e_machine;   /* enum elf_machine */
    Elf64_Word    e_version;   /* enum elf_version */
    Elf64_Addr    e_entry;     /* virtual address of program entry point */
    Elf64_Off     e_phoff;     /* offset to program headers */
    Elf64_Off     e_shoff;     /* offset to section headers */
    Elf64_Word    e_flags;     /* processor specific flags */
    Elf64_Half    e_ehsize;    /* size of elf header */
    Elf64_Half    e_phentsize; /* size of program header entry */
    Elf64_Half    e_phnum;     /* number of program headers */
    Elf64_Half    e_shentsize; /* size of section header entry */
    Elf64_Half    e_shnum;     /* number of section headers */
    Elf64_Half    e_shstrndx;  /* section header index of section names table */
} Elf64_Ehdr;

#define ELF_VERIFY_MAGIC(hdr) \
    ((hdr).e_ident[EI_MAG0] == 0x7f && \
     (hdr).e_ident[EI_MAG1] == 'E' && \
     (hdr).e_ident[EI_MAG2] == 'L' && \
     (hdr).e_ident[EI_MAG3] == 'F')

#define _ENUM_STR(prefix, suffix) [prefix ## suffix] = (#suffix)

/* elf64 table 3 */
enum elf_class {
    ELFCLASSNONE, /* invalid class */
    ELFCLASS32,   /* 32 bit object */
    ELFCLASS64,   /* 64 bit object */
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
    ELFOSABI_LINUX = ELFOSABI_GNU,
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
    _ENUM_STR(ELFOSABI_, LINUX),
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
    ET_NONE, /* invalid type */
    ET_REL,  /* relocatable object */
    ET_EXEC, /* statically linked+loaded executable */
    ET_DYN,  /* shared object */
    ET_CORE, /* core file */
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

/* glibc elf/elf.h */
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

/* implied from description of e_version in elf64 section 3 */
enum elf_version {
    EV_NONE,    /* invalid version */
    EV_CURRENT, /* current version, future versions reserved */
};

static const char *const elf_version_str[] = {
    _ENUM_STR(EV_, NONE),
    _ENUM_STR(EV_, CURRENT),
};

/* elf64 table 7 */
enum elf_section_index {
    SHN_UNDEF,           /* undefined symbol */
    SHN_BEGIN,           /* index of first symbol entry */
    SHN_LOPROC = 0xff00,
    SHN_HIPROC = 0xff1f,
    SHN_LOOS = 0xff20,
    SHN_HIOS = 0xff3f,
    SHN_ABS = 0xfff1,    /* symbol does not reside in a section, but at an
                          * absolute location */
    SHN_COMMON = 0xfff2, /* common symbol */
};

/* elf64 figure 3 */
typedef struct {
    Elf64_Word  sh_name;      /* offset of name in string table */
    Elf64_Word  sh_type;      /* enum elf_section_type */
    Elf64_Xword sh_flags;     /* enum elf_section_flags */
    Elf64_Addr  sh_addr;      /* virtual address of section */
    Elf64_Off   sh_offset;    /* file location of section */
    Elf64_Xword sh_size;      /* size of section in file */
    Elf64_Word  sh_link;      /* rela: symbol table referred to by relocations
                               * symtab: string table of symbol names */
    Elf64_Word  sh_info;      /* rela: section that relocations apply to
                               * symtab: index of first non-local symbol */
    Elf64_Xword sh_addralign; /* address alignment boundary */
    Elf64_Xword sh_entsize;   /* if section is an array of entries, the size of
                               * each entry */
} Elf64_Shdr;

/* elf64 table 8 + glibc elf/elf.h */
enum elf_section_type {
    SHT_NULL,     /* invalid type */
    SHT_PROGBITS, /* initialized code or data */
    SHT_SYMTAB,   /* symbol table: Elf64_Sym array */
    SHT_STRTAB,   /* string table reffered to by a symbol table */
    SHT_RELA,     /* Elf64_Rela */
    SHT_HASH,     /* struct elf_hash_table */
    SHT_DYNAMIC,  /* Elf64_Dyn array */
    SHT_NOTE,     /* eg .note etc */
    SHT_NOBITS,   /* Occupies space in memory but not in file (eg .bss) */
    SHT_REL,      /* Elf64_Rel */
    SHT_RESERVED,
    SHT_DYNSYM,   /* dynamic symbol table */
    SHT_INIT_ARRAY,
    SHT_FINI_ARRAY,
    SHT_PREINIT_ARRAY,
    SHT_GROUP,
    SHT_NUM,      /* number of defined types */
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

/* elf64 table 9 */
enum elf_section_flags {
    SHF_WRITE = 1 << 0,            /* writable */
    SHF_ALLOC = 1 << 1,            /* occupies memory in execution model */
    SHF_EXECINSTR = 1 << 2,        /* executable */
/* glibc elf.elf.h */
    SHF_MERGE = 1 << 3,            /* might be merged */
    SHF_STRINGS = 1 << 5,          /* Contains null-terminated strings */
    SHF_INFO_LINK = 1 << 6,        /* sh_info contains SHT index */
    SHF_LINK_ORDER = 1 << 7,       /* preserve order after combining */
    SHF_OS_NONCONFORMING = 1 << 8, /* non-standard handling required */
    SHF_GROUP = 1 << 9,            /* section is member of a group */
    SHF_TLS = 1 << 10,             /* thread local storage */
    SHF_COMPRESSED = 1 << 11,      /* contains compressed data */
#define SHF_MASKOS 0xff00000
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

/* elf64 figure 5 */
typedef struct {
    Elf64_Addr  r_offset; /* virtual address of reference */
    Elf64_Xword r_info;   /* index and type */
} Elf64_Rel;

typedef struct {
    Elf64_Addr   r_offset; /* virtual address of reference */
    Elf64_Xword  r_info;   /* symbol index and type */
    Elf64_Sxword r_addend; /* constant part of expression */
} Elf64_Rela;

/* r_info */
#define ELF64_R_SYM(rel) ((rel).r_info >> 32)
#define ELF64_R_TYPE(rel) ((rel).r_info & 0xffffffffL)

/* SysV ABI table 4.9 */
enum elf_relocation_type {
    R_X86_64_NONE,
    R_X86_64_64,
    R_X86_64_PC32,
    R_X86_64_GOT32,
    R_X86_64_PLT32,
    R_X86_64_COPY,
    R_X86_64_GLOB_DAT,
    R_X86_64_JUMP_SLOT,
    R_X86_64_RELATIVE,
    R_X86_64_GOTPCREL,
    R_X86_64_32,
    R_X86_64_32S,
    R_X86_64_16,
    R_X86_64_PC16,
    R_X86_64_8,
    R_X86_64_PC8,
    R_X86_64_DTPMOD64,
    R_X86_64_DTPOFF64,
    R_X86_64_TPOFF64,
    R_X86_64_TLSGD,
    R_X86_64_TLSLD,
    R_X86_64_DTPOFF32,
    R_X86_64_GOTTPOFF,
    R_X86_64_TPOFF32,
    R_X86_64_PC64,
    R_X86_64_GOTOFF64,
    R_X86_64_GOTPC32,
    R_X86_64_SIZE32 = 32,
    R_X86_64_SIZE64,
    R_X86_64_GOTPC32_TLSDESC,
    R_X86_64_TLSDESC_CALL,
    R_X86_64_TLSDESC,
    R_X86_64_IRELATIVE,
    R_X86_64_RELATIVE64,
    R_X86_64_DEPRECATED_39,
    R_X86_64_DEPRECATED_40,
    R_X86_64_GOTPCRELX,
    R_X86_64_REX_GOTPCRELX,
};

static const char *const elf_relocation_type_str[] = {
    _ENUM_STR(R_X86_64_, NONE),
    _ENUM_STR(R_X86_64_, 64),
    _ENUM_STR(R_X86_64_, PC32),
    _ENUM_STR(R_X86_64_, GOT32),
    _ENUM_STR(R_X86_64_, PLT32),
    _ENUM_STR(R_X86_64_, COPY),
    _ENUM_STR(R_X86_64_, GLOB_DAT),
    _ENUM_STR(R_X86_64_, JUMP_SLOT),
    _ENUM_STR(R_X86_64_, RELATIVE),
    _ENUM_STR(R_X86_64_, GOTPCREL),
    _ENUM_STR(R_X86_64_, 32),
    _ENUM_STR(R_X86_64_, 32S),
    _ENUM_STR(R_X86_64_, 16),
    _ENUM_STR(R_X86_64_, PC16),
    _ENUM_STR(R_X86_64_, 8),
    _ENUM_STR(R_X86_64_, PC8),
    _ENUM_STR(R_X86_64_, DTPMOD64),
    _ENUM_STR(R_X86_64_, DTPOFF64),
    _ENUM_STR(R_X86_64_, TPOFF64),
    _ENUM_STR(R_X86_64_, TLSGD),
    _ENUM_STR(R_X86_64_, TLSLD),
    _ENUM_STR(R_X86_64_, DTPOFF32),
    _ENUM_STR(R_X86_64_, GOTTPOFF),
    _ENUM_STR(R_X86_64_, TPOFF32),
    _ENUM_STR(R_X86_64_, PC64),
    _ENUM_STR(R_X86_64_, GOTOFF64),
    _ENUM_STR(R_X86_64_, GOTPC32),
    _ENUM_STR(R_X86_64_, SIZE32 ),
    _ENUM_STR(R_X86_64_, SIZE64),
    _ENUM_STR(R_X86_64_, GOTPC32_TLSDESC),
    _ENUM_STR(R_X86_64_, TLSDESC_CALL),
    _ENUM_STR(R_X86_64_, TLSDESC),
    _ENUM_STR(R_X86_64_, IRELATIVE),
    _ENUM_STR(R_X86_64_, RELATIVE64),
    _ENUM_STR(R_X86_64_, DEPRECATED_39),
    _ENUM_STR(R_X86_64_, DEPRECATED_40),
    _ENUM_STR(R_X86_64_, GOTPCRELX),
    _ENUM_STR(R_X86_64_, REX_GOTPCRELX),
};

/* elf64 figure 6 */
typedef struct {
    Elf64_Word  p_type;   /* enum elf_segment_type */
    Elf64_Word  p_flags;  /* enum elf_segment_attributes */
    Elf64_Off   p_offset; /* offset in file */
    Elf64_Addr  p_vaddr;  /* execution virtual address */
    Elf64_Addr  p_paddr;  /* reserved */
    Elf64_Xword p_filesz; /* size of segment in file */
    Elf64_Xword p_memsz;  /* size of segment in memory */
    Elf64_Xword p_align;  /* alignment of segment */
} Elf64_Phdr;

/* elf64 table 16 */
enum elf_segment_type {
    PT_NULL,    /* unused entry */
    PT_LOAD,    /* loadable segment */
    PT_DYNAMIC, /* dynamic linking tables */
    PT_INTERP,  /* requested program interpreter */
    PT_NOTE,    /* note sections */
    PT_SHLIB,   /* reserved */
    PT_PHDR,    /* program header table */
/* glibc elf/elf.h */
    PT_TLS,     /* thread local storage */
    PT_NUM,     /* number of standard types */
#define PT_LOOS 0x60000000
    PT_GNU_STACK = 0x6474e551, /* program needs non-executable stack */
    PT_GNU_RELRO,              /* set this region to RO after relocations */
#define PT_HIOS 0x6fffffff
#define PT_LOPROC 0x70000000
#define PT_HIPROC 0x7fffffff
};

static const char *const elf_segment_type_str[] = {
    _ENUM_STR(PT_, NULL),
    _ENUM_STR(PT_, LOAD),
    _ENUM_STR(PT_, DYNAMIC),
    _ENUM_STR(PT_, INTERP),
    _ENUM_STR(PT_, NOTE),
    _ENUM_STR(PT_, SHLIB),
    _ENUM_STR(PT_, PHDR),
    _ENUM_STR(PT_, TLS),
    [PT_NUM + 0] = "STACK",
    [PT_NUM + 1] = "RELRO",
};

/* elf64 table 17 */
enum elf_segment_attributes {
    PF_X = 1 << 0,
    PF_W = 1 << 1,
    PF_R = 1 << 2,
#define PF_MASKOS 0xff0000
#define PF_MASK_PROC 0xff000000
};

/* elf64 figure 8 */
typedef struct {
    Elf64_Sxword d_tag;    /* enum elf_dyn_tag: determines interpretation of
                            * d_un */
    union {
        Elf64_Xword d_val; /* integer value */
        Elf64_Addr  d_ptr; /* virtual address (pre-relocation) */
    } d_un;
} Elf64_Dyn;

/* elf64 table 18 */
enum elf_dyn_tag {
    DT_NULL,            /* end of array */
    DT_NEEDED,          /* val: offset to name of .so */
    DT_PLTRELSZ,        /* val: size of relocation table associated with plt */
    DT_PLTGOT,          /* ptr: address of .got.plt */
    DT_HASH,            /* ptr: address of hash table */
    DT_STRTAB,          /* ptr: address of dynamic string table */
    DT_SYMTAB,          /* ptr: address of dynamic symbol table */
    DT_RELA,            /* ptr: address of relocation table */
    DT_RELASZ,          /* val: size of relocation table */
    DT_RELAENT,         /* val: size of relocation entry */
    DT_STRSZ,           /* val: size of string table */
    DT_SYMENT,          /* val: size of each symbol table entry */
    DT_INIT,            /* ptr: address of _init function */
    DT_FINI,            /* ptr: address of _fini function */
    DT_SONAME,          /* val: offset to name of this .so */
    DT_RPATH,           /* val: offset to search path string */
    DT_SYMBOLIC,        /* not quite sure what this means */
    DT_REL,             /* ptr: address of relocation table */
    DT_RELSZ,           /* val: size of relocation table */
    DT_RELENT,          /* val: size of relocation entry */
    DT_PLTREL,          /* val: type of relocations in plt, DT_REL[A] */
    DT_DEBUG,           /* ptr: reserved */
    DT_TEXTREL,         /* if present, the relocation table contains relocations
                         * for a non-writable segment */
    DT_JMPREL,          /* ptr: address of relocations associated with plt */
    DT_BIND_NOW,        /* if present, process all relocations before
                         * transferring control to program */
    DT_INIT_ARRAY,      /* ptr: void (*init_func)(void) init_array[] */
    DT_FINI_ARRAY,      /* ptr: void (*fini_func)(void) fini_array[] */
    DT_INIT_ARRAYSZ,    /* val: size of init array */
    DT_FINI_ARRAYSZ,    /* val: size of fini array */
#define DT_MAX DT_FINI_ARRAYSZ
#define DT_LOOS 0x60000000
#define DT_HIOS 0x6fffffff
#define DT_LOPROC 0x70000000
#define DT_HIPROC 0x7fffffff
};

static const char *const elf_dyn_tag_str[] = {
    _ENUM_STR(DT_, NULL),
    _ENUM_STR(DT_, NEEDED),
    _ENUM_STR(DT_, PLTRELSZ),
    _ENUM_STR(DT_, PLTGOT),
    _ENUM_STR(DT_, HASH),
    _ENUM_STR(DT_, STRTAB),
    _ENUM_STR(DT_, SYMTAB),
    _ENUM_STR(DT_, RELA),
    _ENUM_STR(DT_, RELASZ),
    _ENUM_STR(DT_, RELAENT),
    _ENUM_STR(DT_, STRSZ),
    _ENUM_STR(DT_, SYMENT),
    _ENUM_STR(DT_, INIT),
    _ENUM_STR(DT_, FINI),
    _ENUM_STR(DT_, SONAME),
    _ENUM_STR(DT_, RPATH),
    _ENUM_STR(DT_, SYMBOLIC),
    _ENUM_STR(DT_, REL),
    _ENUM_STR(DT_, RELSZ),
    _ENUM_STR(DT_, RELENT),
    _ENUM_STR(DT_, PLTREL),
    _ENUM_STR(DT_, DEBUG),
    _ENUM_STR(DT_, TEXTREL),
    _ENUM_STR(DT_, JMPREL),
    _ENUM_STR(DT_, BIND_NOW),
    _ENUM_STR(DT_, INIT_ARRAY),
    _ENUM_STR(DT_, FINI_ARRAY),
    _ENUM_STR(DT_, INIT_ARRAYSZ),
    _ENUM_STR(DT_, FINI_ARRAYSZ),
};

/* elf64 figure 9 */
struct elf_hash_table {
    Elf64_Word        nbucket;
    Elf64_Word        nchain;
    const Elf64_Word *buckets;
    const Elf64_Word *chains;
    const Elf64_Sym  *symbols;
    const char       *strings;
};

/* elf64 figure 9 */
void init_hash_table(struct elf_hash_table *hash_table, void *addr,
                     const Elf64_Sym *symbols, const char *strings);
