/* this module provides functionality to handle x86 interrupts */
#pragma once

/* initialize the idt and load it into the idtr */
void install_idt(void);

struct interrupt_frame;
void interrupt_handler(struct interrupt_frame*);
