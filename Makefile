TARGET := i686-elf
PREFIX := cross/bin
AS := $(PREFIX)/$(TARGET)-as
CC := $(PREFIX)/$(TARGET)-gcc
CFLAGS += -std=gnu99 -Wall -Wextra -ffreestanding -nostdlib -O2

.PHONY: all
all: SctOs

SctOs: linker.ld boot.o kernel.o terminal.o
	$(CC) $(CFLAGS) -o $@ -T $^ -lgcc

.PHONY: clean run deps

clean:
	rm -rfv boot.o kernel.o SctOs

run: SctOs
	qemu-system-i386 -kernel $<

deps:
	$(CC) $(CFLAGS) -MM *.c | tee -a Makefile

# deps:
kernel.o: kernel.c vga.h string.h terminal.h
terminal.o: terminal.c terminal.h string.h vga.h
