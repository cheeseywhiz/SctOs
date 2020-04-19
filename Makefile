SOURCE_TREE := $(shell find boot src -type d)
ASM_SOURCES := $(foreach dir, $(SOURCE_TREE), $(wildcard $(dir)/*.s))
ASM_OBJ := $(ASM_SOURCES:%.s=%.o)
C_SOURCES := $(foreach dir, $(SOURCE_TREE), $(wildcard $(dir)/*.c))
C_OBJ := $(C_SOURCES:%.c=%.o)
OBJECTS := $(ASM_OBJ) $(C_OBJ)
BUILD := build
OBJECTS := $(addprefix $(BUILD)/, $(OBJECTS))
DEPS := $(OBJECTS:%.o=%.d)
BUILD_TREE := $(BUILD) $(addprefix $(BUILD)/, $(SOURCE_TREE))

CC := $(TARGET)-gcc
AS := $(CC)
CFLAGS += -std=gnu99 -Werror -Wall -Wextra -pedantic -ffreestanding -O2

.SUFFIXES:

.PHONY: all
all: $(BUILD_TREE) $(BUILD)/opsys

$(BUILD_TREE):
	mkdir -p $@

$(BUILD)/opsys: linker.ld $(OBJECTS)
	$(CC) $(CFLAGS) -nostdlib -T linker.ld -o $@ $(OBJECTS) -lgcc

$(BUILD)/%.o: %.s
	$(AS) -c -o $@ $<

$(BUILD)/%.o: %.c
	$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

-include $(DEPS)

.PHONY: clean run debug-deps

clean:
	-rm -rf $(BUILD)

run: all
	-qemu-system-i386 -kernel $(BUILD)/opsys
