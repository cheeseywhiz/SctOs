#pragma once
#include "util.h"
struct bootloader_data;
typedef void kernel_main_t(struct bootloader_data*);
#ifdef _KERNEL
__noreturn kernel_main_t kernel_main;
#endif
