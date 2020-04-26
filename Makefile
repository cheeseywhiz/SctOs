BUILD := build
LINKER_SCRIPT := linker.ld

TARGET_CC := ./cross/bin/i686-elf-gcc
TARGET_AS := ./cross/bin/i686-elf-as
CFLAGS += -std=gnu99 -O2
CFLAGS += -Werror -Wall -Wextra -pedantic
CPPFLAGS += -Iinclude
TARGET_CFLAGS += -ffreestanding
LDLIBS += -lgcc

KERNEL_DIRS := lib src
KERNEL_TREE := $(shell find $(KERNEL_DIRS) -type d)
KERNEL_ASM_SOURCES := $(shell find $(KERNEL_DIRS) -name "*.s")
KERNEL_C_SOURCES := $(shell find $(KERNEL_DIRS) -name "*.c")
KERNEL_OBJECTS := $(KERNEL_ASM_SOURCES:%.s=%.o) $(KERNEL_C_SOURCES:%.c=%.o)
KERNEL_OBJECTS := $(addprefix $(BUILD)/target/, $(KERNEL_OBJECTS))
KERNEL_TREE := $(addprefix $(BUILD)/target/, $(KERNEL_TREE))

TEST_TREE := lib $(shell find test -type d)
TEST_SOURCES := lib/readelf.c $(shell find test -name "*.c")
TEST_OBJECTS := $(TEST_SOURCES:%.c=%.o)
TEST_OBJECTS := $(addprefix $(BUILD)/host/, $(TEST_OBJECTS))
TEST_TREE := $(addprefix $(BUILD)/host/, $(TEST_TREE))

DEPS := $(KERNEL_OBJECTS) $(TEST_OBJECTS)
DEPS := $(DEPS:%.o=%.d)
BUILD_TREE := $(BUILD) $(BUILD)/target $(KERNEL_TREE) $(BUILD)/host $(TEST_TREE)

.SUFFIXES:

.PHONY: all
all: kernel

$(BUILD_TREE):
	@mkdir -p $@

$(BUILD)/target/opsys: $(LINKER_SCRIPT) $(KERNEL_OBJECTS)
	$(TARGET_CC) $(CFLAGS) $(TARGET_CFLAGS) -nostdlib -T $(LINKER_SCRIPT) -o $@ $(KERNEL_OBJECTS) $(LDLIBS)

$(BUILD)/target/%.o: %.s
	$(TARGET_AS) -o $@ $<

$(BUILD)/target/%.o: %.c
	$(TARGET_CC) $(CPPFLAGS) $(CFLAGS) $(TARGET_CFLAGS) -MMD -MP -c -o $@ $<

$(BUILD)/host/test/readelf: $(BUILD)/host/test/readelf.o $(BUILD)/host/lib/readelf.o
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD)/host/%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c -o $@ $<

-include $(DEPS)

.PHONY: build_tree kernel tests clean qemu

build-tree: $(BUILD_TREE)

kernel: build-tree $(BUILD)/target/opsys

tests: build-tree $(BUILD)/host/test/readelf

clean:
	-rm -rf $(BUILD)

qemu: all
	-qemu-system-i386 -kernel $(BUILD)/target/opsys
