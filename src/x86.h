#pragma once
#include "opsys/x86.h"
#include "virtual-memory.h"

/* everything that is on a per-cpu basis */
struct x86_64_cpu {
    struct {
        uint64_t paddr;
        uint64_t vaddr;
    } apic;
    page_table_t *address_space;
};

extern struct x86_64_cpu cpu;

void init_cpu(void);
void init_apic(void);
