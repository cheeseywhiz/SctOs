# test build is in tree, kernel and efi loader are out of tree
BUILD_TARGET ?= build/target
BUILD_EFI ?= build/efi
BUILD_HOST ?= .
BUILD_TREE += build

OVMF_CODE := /usr/share/edk2-ovmf/x64/OVMF_CODE.fd
OVMF_VARS := /usr/share/edk2-ovmf/x64/OVMF_VARS.fd
BUILD_OVMF_VARS := $(BUILD_EFI)/OVMF_VARS.fd
DISK := $(BUILD_EFI)/disk.img

TARGET_CC := ./cross/bin/i686-elf-gcc
TARGET_AS := ./cross/bin/i686-elf-as

CFLAGS += -std=gnu11 -O2
CPPFLAGS += -iquote include -fdiagnostics-color=always
CFLAGS += -Werror -Wall -Wextra -pedantic -Wshadow -Wpointer-arith \
	-Wcast-align -Wwrite-strings -Wmissing-prototypes -Wmissing-declarations \
	-Wredundant-decls -Wnested-externs -Winline -Wno-long-long -Wconversion \
	-Wstrict-prototypes
OBJECT_CFLAGS += -MMD -MP
KERNEL_CFLAGS += -ffreestanding -Wl,-z,separate-code
KERNEL_LDSCRIPT := linker.ld
KERNEL_LDFLAGS += -nostdlib -T $(KERNEL_LDSCRIPT)
KERNEL_LDLIBS := -lgcc
EFI_CPPFLAGS += -I/usr/include/efi -I/usr/include/efi/x86_64 \
	-DGNU_EFI_USE_MS_ABI
EFI_CFLAGS += -mno-red-zone -mno-avx -fshort-wchar -fno-strict-aliasing \
	-ffreestanding -fno-stack-protector -fno-merge-constants -fPIC \
	-Wno-write-strings -Wno-redundant-decls -Wno-strict-prototypes
EFI_CRT := /usr/lib/crt0-efi-x86_64.o
EFI_LDSCRIPT := /usr/lib/elf_x86_64_efi.lds
EFI_LDFLAGS += -nostdlib -shared -T $(EFI_LDSCRIPT) \
	-Wl,-Bsymbolic,--warn-common,--defsym=EFI_SUBSYSTEM=0xa,--no-undefined \
	-Wl,--fatal-warnings,--build-id=sha1,-z,nocombreloc
EFI_LDLIBS := -lefi -lgnuefi -lgcc
TEST_CFLAGS += -O0 -ggdb
TEST_LDFLAGS += -Wl,--hash-style=sysv

KERNEL_DIRS := lib src
KERNEL_TREE := $(shell find $(KERNEL_DIRS) -type d)
KERNEL_ASM_SOURCES := $(shell find $(KERNEL_DIRS) -name "*.s")
KERNEL_C_SOURCES := $(shell find $(KERNEL_DIRS) -name "*.c")
KERNEL_SOURCES := $(KERNEL_ASM_SOURCES) $(KERNEL_C_SOURCES)
KERNEL_OBJECTS := $(KERNEL_ASM_SOURCES:%.s=%.o) $(KERNEL_C_SOURCES:%.c=%.o)
KERNEL_OBJECTS := $(addprefix $(BUILD_TARGET)/, $(KERNEL_OBJECTS))
KERNEL_TREE := $(addprefix $(BUILD_TARGET)/, $(KERNEL_TREE))
KERNEL := $(BUILD_TARGET)/opsys

EFI_DIRS := efi
EFI_TREE := $(shell find $(EFI_DIRS) -type d)
EFI_SOURCES := $(shell find $(EFI_DIRS) -name "*.c")
EFI_OBJECTS := $(EFI_SOURCES:%.c=%.o)
EFI_OBJECTS := $(addprefix $(BUILD_EFI)/, $(EFI_OBJECTS))
EFI_TREE := $(addprefix $(BUILD_EFI)/, $(EFI_TREE))
EFI_SO := $(BUILD_EFI)/opsys-loader.so
EFI_EXEC := $(BUILD_EFI)/opsys-loader.efi

TEST_TREE := $(shell find test lib -type d)
TEST_SOURCES := $(shell find lib test -name "*.c")
TEST_OBJECTS := $(TEST_SOURCES:%.c=%.o)
TEST_OBJECTS := $(addprefix $(BUILD_HOST)/, $(TEST_OBJECTS))
TEST_TREE := $(addprefix $(BUILD_HOST)/, $(TEST_TREE))
TEST_EXECS := $(addprefix $(BUILD_HOST)/, readelf small-exec introspect)

SOURCES := $(KERNEL_SOURCES) $(EFI_SOURCES) $(TEST_SOURCES)
OBJECTS := $(KERNEL_OBJECTS) $(EFI_OBJECTS) $(TEST_OBJECTS)
DEPS := $(OBJECTS:%.o=%.d)
BUILD_TREE += $(BUILD_TARGET) $(BUILD_EFI) $(BUILD_HOST) $(KERNEL_TREE) \
	$(EFI_TREE) $(TEST_TREE)

$(OBJECTS): CFLAGS += $(OBJECT_CFLAGS)
$(BUILD_HOST)/lib/%.o: CFLAGS += -ffreestanding

.SUFFIXES:

.PHONY: kernel
kernel: $(KERNEL_TREE) $(KERNEL)

$(BUILD_TREE):
	mkdir -p $@

$(KERNEL): $(KERNEL_LDSCRIPT) $(KERNEL_OBJECTS)
	$(TARGET_CC) $(CFLAGS) $(KERNEL_CFLAGS) $(KERNEL_LDFLAGS) -o $@ \
		$(KERNEL_OBJECTS) $(KERNEL_LDLIBS)

$(BUILD_TARGET)/%.o: %.s
	$(TARGET_AS) -o $@ $<

$(BUILD_TARGET)/%.o: %.c
	$(TARGET_CC) $(CPPFLAGS) $(CFLAGS) $(KERNEL_CFLAGS) -c -o $@ $<

$(EFI_EXEC): $(EFI_SO) $(EFI_TREE)
	objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel \
		-j .rela -j .rel.* -j .rela.* -j .rel* -j .rela* -j .reloc \
		--target=efi-app-x86_64 $< $@

$(EFI_SO): $(EFI_CRT) $(EFI_LDSCRIPT) $(EFI_OBJECTS)
	$(CC) $(EFI_LDFLAGS) -o $@ $(EFI_CRT) $(EFI_OBJECTS) $(EFI_LDLIBS)

$(BUILD_EFI)/%.o: %.c
	$(CC) $(CPPFLAGS) $(EFI_CPPFLAGS) $(CFLAGS) $(EFI_CFLAGS) -c -o $@ $<

$(BUILD_OVMF_VARS): $(OVMF_VARS)
	install -m 644 $< $@

$(DISK): $(EFI_EXEC)
	cp empty-efi-disk.img $@
	chmod +w $@
	./make-disk-img.sh $@ $^

$(BUILD_HOST)/readelf: $(addprefix $(BUILD_HOST)/, lib/readelf.o \
		test/glibc-readelf.o test/readelf.o test/readelf-main.o)
	$(CC) $(CFLAGS) $(TEST_CFLAGS) $(TEST_LDFLAGS) -o $@ $^

$(BUILD_HOST)/%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(TEST_CFLAGS) -c -o $@ $<

$(BUILD_HOST)/small-exec: test/small-exec.S
	$(CPP) $< | $(AS) -o $@.o -
	$(LD) -nostdlib -o $@ $@.o
	-$(RM) $@.o

$(BUILD_HOST)/introspect: $(addprefix $(BUILD_HOST)/, lib/elf.o \
		test/glibc-readelf.o test/readelf.o test/introspect.o)
	$(CC) $(CFLAGS) $(TEST_CFLAGS) $(HOST_LDFLAGS) -o $@ $^

tags: $(SOURCES) $(shell find . -name "*.h" -not -path "./cross/*")
	ctags --exclude=cross/\* --exclude=\*.json --exclude=Makefile -R .

-include $(DEPS)

.PHONY: all build-tree efi tests clean qemu-x86 qemu-x64

all: tests kernel efi

build-tree: $(BUILD_TREE)

efi: $(EFI_TREE) $(EFI_EXEC)

tests: $(TEST_TREE) $(TEST_EXECS)

clean:
	@-$(RM) $(OBJECTS) $(DEPS) $(KERNEL) $(EFI_SO) $(EFI_EXEC) $(TEST_EXECS) \
		$(BUILD_OVMF_VARS) $(DISK) vgcore.* perf.*
	@for f in $(shell echo $(BUILD_TREE) | tr ' ' '\n' | sort -r); do \
		rmdir $$f 1>/dev/null 2>&1 || true; \
	done

qemu-x86: kernel
	qemu-system-i386 -kernel $(BUILD_TARGET)/opsys &

qemu-x64: $(BUILD_OVMF_VARS) $(DISK)
	qemu-system-x86_64 \
		-cpu qemu64 \
		-drive if=pflash,format=raw,unit=0,file=$(OVMF_CODE),readonly=on \
		-drive if=pflash,format=raw,unit=1,file=$(BUILD_OVMF_VARS) \
		-drive file=$(DISK),if=ide,format=raw \
		-net none \
		&
