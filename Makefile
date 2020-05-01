# in tree test build:
BUILD_TARGET ?= build
BUILD_HOST ?= .
# in tree kernel build:
BUILD_TARGET ?= .
BUILD_HOST ?= build
# out of tree build:
BUILD_TARGET ?= build/target
BUILD_HOST ?= build/host

TARGET_CC := ./cross/bin/i686-elf-gcc
TARGET_AS := ./cross/bin/i686-elf-as
CFLAGS += -std=gnu99 -O2
CFLAGS += -Werror -Wall -Wextra -pedantic
OBJECT_CFLAGS += -MMD -MP
CPPFLAGS += -iquote include
KERNEL_CFLAGS += -ffreestanding
TEST_CFLAGS += -Og -ggdb
TEST_OBJECT_CFLAGS += -fpie
KERNEL_LDLIBS += -lgcc
LINKER_SCRIPT := linker.ld

KERNEL_DIRS := lib src
KERNEL_TREE := $(shell find $(KERNEL_DIRS) -type d)
KERNEL_ASM_SOURCES := $(shell find $(KERNEL_DIRS) -name "*.s")
KERNEL_C_SOURCES := $(shell find $(KERNEL_DIRS) -name "*.c")
KERNEL_OBJECTS := $(KERNEL_ASM_SOURCES:%.s=%.o) $(KERNEL_C_SOURCES:%.c=%.o)
KERNEL_OBJECTS := $(addprefix $(BUILD_TARGET)/, $(KERNEL_OBJECTS))
KERNEL_TREE := $(addprefix $(BUILD_TARGET)/, $(KERNEL_TREE))
KERNEL := $(BUILD_TARGET)/opsys

TEST_TREE := $(shell find test lib -type d)
TEST_SOURCES := $(shell find lib test -name "*.c")
TEST_OBJECTS := $(TEST_SOURCES:%.c=%.o)
TEST_OBJECTS := $(addprefix $(BUILD_HOST)/, $(TEST_OBJECTS))
TEST_TREE := $(addprefix $(BUILD_HOST)/, $(TEST_TREE))
TEST_EXECS := $(addprefix $(BUILD_HOST)/, readelf small-exec)

OBJECTS := $(KERNEL_OBJECTS) $(TEST_OBJECTS)
DEPS := $(OBJECTS:%.o=%.d)
BUILD_TREE := $(BUILD_TARGET) $(BUILD_HOST) $(KERNEL_TREE) $(TEST_TREE)

$(OBJECTS): CFLAGS += $(OBJECT_CFLAGS)
$(BUILD_TARGET)/%.o: CFLAGS += $(KERNEL_CFLAGS)
$(TEST_OBJECTS): CFLAGS += $(TEST_OBJECT_CFLAGS)
$(BUILD_HOST)/lib/%.o: CFLAGS += -nostdlib -ffreestanding
$(BUILD_HOST)/%.o: CFLAGS += $(TEST_CFLAGS)

.SUFFIXES:

.PHONY: kernel
kernel: build-tree $(KERNEL)

$(BUILD_TREE):
	@mkdir -p $@

$(KERNEL): $(LINKER_SCRIPT) $(KERNEL_OBJECTS)
	$(TARGET_CC) $(CFLAGS) $(KERNEL_CFLAGS) -nostdlib -T $(LINKER_SCRIPT) -o $@ $(KERNEL_OBJECTS) $(KERNEL_LDLIBS)

$(BUILD_TARGET)/%.o: %.s
	$(TARGET_AS) -o $@ $<

$(BUILD_TARGET)/%.o: %.c
	$(TARGET_CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(BUILD_HOST)/readelf: $(BUILD_HOST)/test/readelf.o $(BUILD_HOST)/lib/string.o
	$(CC) $(CFLAGS) $(TEST_CFLAGS) -static-pie -o $@ $^

$(BUILD_HOST)/%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(BUILD_HOST)/small-exec: test/small-exec.S
	$(CPP) $< | $(AS) -o $@.o -
	$(LD) -nostdlib -o $@ $@.o
	-$(RM) $@.o

-include $(DEPS)

.PHONY: all build-tree kernel tests clean qemu

all: kernel tests

build-tree: $(BUILD_TREE)

tests: build-tree $(TEST_EXECS)

clean:
	-$(RM) $(OBJECTS) $(DEPS) $(KERNEL) $(TEST_EXECS)
	-for f in $(shell echo $(BUILD_TREE) | tr ' ' '\n' | sort -r); do \
		rmdir $$f 2>/dev/null; \
	done

qemu: all
	-qemu-system-i386 -kernel $(BUILD_TARGET)/opsys
