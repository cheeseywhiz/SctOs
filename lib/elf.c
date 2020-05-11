/* this file contains helper functions that pertain to the elf spec */
#include "elf.h"
#include "string.h"

struct hash_table_lengths {
    Elf64_Word nbucket;
    Elf64_Word nchain;
};

/* elf64 figure 9 */
void
init_hash_table(struct elf_hash_table *hash_table, void *addr,
                const Elf64_Sym *symbols, const char *strings)
{
    struct hash_table_lengths hash_table_lengths =
        *(struct hash_table_lengths*)addr;
    hash_table->nbucket = hash_table_lengths.nbucket;
    hash_table->nchain = hash_table_lengths.nchain;
    hash_table->buckets =
        (Elf64_Word*)((Elf64_Addr)addr + sizeof(struct hash_table_lengths));
    hash_table->chains = &hash_table->buckets[hash_table->nbucket];
    hash_table->symbols = symbols;
    hash_table->strings = strings;
}

/* elf64 figure 10 */
/* haven't needed to use this yet (& its a static)
static unsigned long
elf64_hash(const unsigned char *name)
{
    unsigned long h = 0, g;

    while (*name) {
        h = (h << 4) + *name++;
        if ((g = h & 0xf0000000))
            h ^= g >> 24;
        h &= 0x0fffffff;
    }

    return h;
}*/
