#pragma once
#include <stdint.h>
extern uint64_t gdt[];
extern uint16_t gdt_length;
enum gdt_index {
    GDTI_DUMMY,
    GDTI_KERNEL_CODE,
    GDTI_KERNEL_DATA,
};
