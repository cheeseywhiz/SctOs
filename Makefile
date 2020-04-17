AS := $(TARGET)-as
CC := $(TARGET)-gcc
CFLAGS += -std=gnu99 -Wall -Wextra -ffreestanding -nostdlib -O2
ASM_SOURCES := $(wildcard *.s)
ASM_OBJ := $(ASM_SOURCES:%.s=%.o)
C_SOURCES := $(wildcard *.c)
C_OBJ := $(C_SOURCES:%.c=%.o)
OBJECTS := $(ASM_OBJ) $(C_OBJ)

.PHONY: all
all: SctOs

SctOs: linker.ld $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ -T $< $(OBJECTS) -lgcc

.PHONY: clean run deps

clean:
	rm -rfv $(OBJECTS) SctOs

run: SctOs
	qemu-system-i386 -kernel $<

deps:
	$(CC) $(CFLAGS) -MM $(C_SOURCES) | tee -a Makefile

# deps:
kernel.o: kernel.c vga.h string.h terminal.h
terminal.o: terminal.c terminal.h string.h vga.h
vga.o: vga.c vga.h
