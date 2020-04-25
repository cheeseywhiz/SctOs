SOURCE_TREE := $(shell find lib src -type d)
ASM_SOURCES := $(foreach dir, $(SOURCE_TREE), $(wildcard $(dir)/*.s))
ASM_OBJ := $(ASM_SOURCES:%.s=%.o)
C_SOURCES := $(foreach dir, $(SOURCE_TREE), $(wildcard $(dir)/*.c))
C_OBJ := $(C_SOURCES:%.c=%.o)
OBJECTS := $(ASM_OBJ) $(C_OBJ)
BUILD := build
OBJECTS := $(addprefix $(BUILD)/, $(OBJECTS))
DEPS := $(OBJECTS:%.o=%.d)
BUILD_TREE := $(BUILD) $(addprefix $(BUILD)/, $(SOURCE_TREE))
LINKER_SCRIPT := linker.ld

LIB_TREE := $(shell find lib -type d)
LIB_SOURCES := $(foreach dir, $(LIB_TREE), $(wildcard $(dir)/*.c))
LIB_OBJECTS := $(LIB_SOURCES:%.c=%.o)
LIB_OBJECTS := $(addprefix $(BUILD)/, $(LIB_OBJECTS))

CC := $(TARGET)-gcc
AS := $(CC)
CFLAGS += -std=gnu99 -ffreestanding -O2
CFLAGS += -Werror -Wall -Wextra -pedantic
CPPFLAGS += -Iinclude
LDLIBS += -lgcc

.SUFFIXES:

.PHONY: all
all: kernel libc

$(BUILD_TREE):
	@mkdir -p $@

$(BUILD)/opsys: $(LINKER_SCRIPT) $(OBJECTS)
	$(CC) $(CFLAGS) -nostdlib -T $(LINKER_SCRIPT) -o $@ $(OBJECTS) $(LDLIBS)

$(BUILD)/libc.a: $(LIB_OBJECTS)
	$(AR) rcs $@ $^

$(BUILD)/%.o: %.s
	$(AS) -c -o $@ $<

$(BUILD)/%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c -o $@ $<

-include $(DEPS)

.PHONY: build_tree kernel libc clean run debug-deps

build_tree: $(BUILD_TREE)

kernel: build_tree $(BUILD)/opsys

libc: build_tree $(BUILD)/libc.a

clean:
	-rm -rf $(BUILD)

run: all
	-qemu-system-i386 -kernel $(BUILD)/opsys
