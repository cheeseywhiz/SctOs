BUILD := build
LINKER_SCRIPT := linker.ld

TARGET_CC := ./cross/bin/i686-elf-gcc
TARGET_AS := ./cross/bin/i686-elf-as
CFLAGS += -std=gnu99 -O2
CFLAGS += -Werror -Wall -Wextra -pedantic
CPPFLAGS += -iquote include
KERNEL_CFLAGS += -ffreestanding
TEST_CFLAGS += -Og -ggdb
OBJECT_CFLAGS += -MMD -MP
LDLIBS += -lgcc

KERNEL_DIRS := lib src
KERNEL_TREE := $(shell find $(KERNEL_DIRS) -type d)
KERNEL_ASM_SOURCES := $(shell find $(KERNEL_DIRS) -name "*.s")
KERNEL_C_SOURCES := $(shell find $(KERNEL_DIRS) -name "*.c")
KERNEL_OBJECTS := $(KERNEL_ASM_SOURCES:%.s=%.o) $(KERNEL_C_SOURCES:%.c=%.o)
KERNEL_OBJECTS := $(addprefix $(BUILD)/target/, $(KERNEL_OBJECTS))
KERNEL_TREE := $(addprefix $(BUILD)/target/, $(KERNEL_TREE))
KERNEL := $(BUILD)/target/opsys

TEST_TREE := $(shell find test -type d)
TEST_SOURCES := $(shell find test -name "*.c")
TEST_OBJECTS := $(TEST_SOURCES:%.c=%.o)
TEST_OBJECTS := $(addprefix $(BUILD)/host/, $(TEST_OBJECTS))
TEST_TREE := $(addprefix $(BUILD)/host/, $(TEST_TREE))
TEST_EXECS := $(addprefix $(BUILD)/host/test/, readelf small-exec)

OBJECTS := $(KERNEL_OBJECTS) $(TEST_OBJECTS)
DEPS := $(KERNEL_OBJECTS) $(TEST_OBJECTS)
DEPS := $(DEPS:%.o=%.d)
BUILD_TREE := $(BUILD) $(BUILD)/target $(KERNEL_TREE) $(BUILD)/host $(TEST_TREE)

$(KERNEL_OBJECTS) $(KERNEL): CFLAGS += $(KERNEL_CFLAGS)
$(TEST_OBJECTS) $(TEST_EXECS): CFLAGS += $(TEST_CFLAGS)
$(OBJECTS): CFLAGS += $(OBJECT_CFLAGS)

.SUFFIXES:

.PHONY: all
all: kernel

$(BUILD_TREE):
	@mkdir -p $@

$(KERNEL): $(LINKER_SCRIPT) $(KERNEL_OBJECTS)
	$(TARGET_CC) $(CFLAGS) -nostdlib -T $(LINKER_SCRIPT) -o $@ $(KERNEL_OBJECTS) $(LDLIBS)

$(BUILD)/target/%.o: %.s
	$(TARGET_AS) -o $@ $<

$(BUILD)/target/%.o: %.c
	$(TARGET_CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(BUILD)/host/test/readelf: $(BUILD)/host/test/readelf.o
	$(CC) $(CFLAGS) -static -o $@ $(BUILD)/host/test/readelf.o

$(BUILD)/host/%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(BUILD)/host/test/small-exec: test/small-exec.S
	$(CPP) $< | $(AS) -o $@.o -
	$(LD) -nostdlib -o $@ $@.o

-include $(DEPS)

.PHONY: build_tree kernel tests clean qemu

build-tree: $(BUILD_TREE)

kernel: build-tree $(KERNEL)

tests: build-tree $(TEST_EXECS)

clean:
	-rm -rf $(BUILD)

qemu: all
	-qemu-system-i386 -kernel $(BUILD)/target/opsys
