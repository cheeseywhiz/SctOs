/* this module provides functionality to handle x86 interrupts */
#pragma once
#include <stdint.h>
struct interrupt_frame;
void interrupt_handler(struct interrupt_frame*, uint64_t);
