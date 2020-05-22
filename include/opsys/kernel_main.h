#pragma once
struct bootloader_data;
typedef void kernel_main_t(struct bootloader_data*);
#ifdef _KERNEL
kernel_main_t kernel_main;
#endif
