BUILD := build
LINKER_SCRIPT := linker.ld

TARGET_CC := ./cross/bin/i686-elf-gcc
TARGET_AS := ./cross/bin/i686-elf-gcc
CFLAGS += -std=gnu99 -ffreestanding -O2
CFLAGS += -Werror -Wall -Wextra -pedantic
CPPFLAGS += -Iinclude
LDLIBS += -lgcc

KERNEL_DIRS := lib src
KERNEL_TREE := $(shell find $(KERNEL_DIRS) -type d)
KERNEL_ASM_SOURCES := $(shell find $(KERNEL_DIRS) -name "*.s")
KERNEL_C_SOURCES := $(shell find $(KERNEL_DIRS) -name "*.c")
KERNEL_OBJECTS := $(KERNEL_ASM_SOURCES:%.s=%.o) $(KERNEL_C_SOURCES:%.c=%.o)
KERNEL_OBJECTS := $(addprefix $(BUILD)/target/, $(KERNEL_OBJECTS))
KERNEL_TREE := $(addprefix $(BUILD)/target/, $(KERNEL_TREE))

DEPS := $(KERNEL_OBJECTS:%.o=%.d)
BUILD_TREE := $(BUILD) $(BUILD)/target $(KERNEL_TREE)

.SUFFIXES:

.PHONY: all
all: kernel

$(BUILD_TREE):
	@mkdir -p $@

$(BUILD)/target/opsys: $(LINKER_SCRIPT) $(KERNEL_OBJECTS)
	$(TARGET_CC) $(CFLAGS) -nostdlib -T $(LINKER_SCRIPT) -o $@ $(KERNEL_OBJECTS) $(LDLIBS)

$(BUILD)/target/%.o: %.s
	$(TARGET_AS) -c -o $@ $<

$(BUILD)/target/%.o: %.c
	$(TARGET_CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c -o $@ $<

-include $(DEPS)

.PHONY: build_tree kernel clean run debug-deps

build-tree: $(BUILD_TREE)

kernel: build-tree $(BUILD)/target/opsys

clean:
	-rm -rf $(BUILD)

qemu: all
	-qemu-system-i386 -kernel $(BUILD)/target/opsys
