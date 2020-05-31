#pragma once
#include <stdint.h>
#include "opsys/kernel_main.h"
typedef void main2_t(void);
extern void setup_new_stack(main2_t, void*);
extern void init_segment_selectors(uint16_t kdata, uint16_t kcode);
