CC := $(TARGET)-gcc
AS := $(CC)
CFLAGS += -std=gnu99 -Werror -Wall -Wextra -pedantic -ffreestanding -O2
ASM_SOURCES := $(wildcard *.s)
ASM_OBJ := $(ASM_SOURCES:%.s=%.o)
C_SOURCES := $(wildcard *.c)
C_OBJ := $(C_SOURCES:%.c=%.o)
OBJECTS := $(ASM_OBJ) $(C_OBJ)

%.o: %.s
	$(AS) -c -o $@ $<

.PHONY: all
all: opsys

opsys: linker.ld $(OBJECTS)
	$(CC) $(CFLAGS) -nostdlib -T $< -o $@ $(OBJECTS) -lgcc

.PHONY: clean run deps

clean:
	rm -rfv $(OBJECTS) opsys

run: opsys
	qemu-system-i386 -kernel $<

deps:
	$(CC) $(CFLAGS) -MM $(C_SOURCES) | tee -a Makefile

# deps:
kernel.o: kernel.c string.h terminal.h vga.h
string.o: string.c string.h terminal.h vga.h
terminal.o: terminal.c terminal.h string.h vga.h
vga.o: vga.c vga.h
