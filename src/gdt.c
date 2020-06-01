#include <stdint.h>
#include "opsys/x86.h"
#include "util.h"
#include "gdt.h"

uint64_t gdt[] = {
    /* dummy */
    0,
    /* kernel code (implicit base=0 and limit=0xfffff) */
    SEGDESC_SET_DPL(0)
        /* code/data seg, 64-bit code seg, present, 4kb granularity */
        | SEGDESC_S | SEGDESC_L | SEGDESC_P | SEGDESC_G
        /* executable+read */
        | SDT_X | SDT_XR
    ,
    /* kernel data */
    SEGDESC_SET_DPL(0)
        /* code/data seg, present, 4kb granularity */
        | SEGDESC_S | SEGDESC_P | SEGDESC_G
        /* read+write */
        | SDT_RW
    ,
};

uint16_t gdt_length = (uint16_t)ARRAY_LENGTH(gdt);
