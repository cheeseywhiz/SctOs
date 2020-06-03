#pragma once
#include "opsys/x86.h"

/* everything that is on a per-cpu basis */
struct x86_64_cpu {
    struct {
        uint64_t paddr;
        uint64_t vaddr;
        uint8_t  id;
    } apic;
};

extern struct x86_64_cpu cpu;

void init_cpu(void);
