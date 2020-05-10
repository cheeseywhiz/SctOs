# test build is in tree, kernel and efi loader are out of tree
BUILD_KERNEL ?= build/kernel
BUILD_EFI ?= build/efi
BUILD_TEST ?= .
BUILD_TREE += build

OVMF_CODE := /usr/share/edk2-ovmf/x64/OVMF_CODE.fd
OVMF_VARS := /usr/share/edk2-ovmf/x64/OVMF_VARS.fd
BUILD_OVMF_VARS := $(BUILD_EFI)/OVMF_VARS.fd
DISK := $(BUILD_EFI)/disk.img

# default rule needs to be pretty high up
.PHONY: disk
disk: $(DISK)

KERNEL_CC := ./cross/bin/x86_64-elf-gcc
KERNEL_AS := ./cross/bin/x86_64-elf-as

CFLAGS += -std=gnu11 -O2
CPPFLAGS += -iquote include -fdiagnostics-color=always
CFLAGS += -Werror -Wall -Wextra -pedantic -Wshadow -Wpointer-arith \
	-Wcast-align -Wwrite-strings -Wmissing-prototypes -Wmissing-declarations \
	-Wredundant-decls -Wnested-externs -Winline -Wno-long-long -Wconversion \
	-Wstrict-prototypes
OBJECT_CFLAGS += -MMD -MP
KERNEL_CFLAGS += -ffreestanding -fPIE
KERNEL_LDFLAGS += -nostdlib -static-pie -Wl,-static,-pie,--no-dynamic-linker \
	-Wl,-z,separate-code,-z,max-page-size=0x1000,-z,noexecstack,-z,relro
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
KERNEL_ASM_SOURCES := $(shell find $(KERNEL_DIRS) -name "*.s")
KERNEL_C_SOURCES := $(shell find $(KERNEL_DIRS) -name "*.c")
KERNEL_SOURCES := $(KERNEL_ASM_SOURCES) $(KERNEL_C_SOURCES)
KERNEL_OBJECTS := $(KERNEL_ASM_SOURCES:%.s=%.o) $(KERNEL_C_SOURCES:%.c=%.o)
KERNEL_OBJECTS := $(addprefix $(BUILD_KERNEL)/, $(KERNEL_OBJECTS))
KERNEL := $(BUILD_KERNEL)/opsys
KERNEL_TREE := $(shell find $(KERNEL_DIRS) -type d)
KERNEL_TREE := $(BUILD_KERNEL) $(addprefix $(BUILD_KERNEL)/, $(KERNEL_TREE))
$(KERNEL) $(KERNEL_OBJECTS): | kernel-tree

EFI_DIRS := efi
EFI_SOURCES := $(shell find $(EFI_DIRS) -name "*.c")
EFI_OBJECTS := $(addprefix $(BUILD_EFI)/, $(EFI_SOURCES:%.c=%.o))
EFI_SO := $(BUILD_EFI)/opsys-loader.so
EFI_EXEC := $(BUILD_EFI)/opsys-loader.efi
EFI_TREE := $(shell find $(EFI_DIRS) -type d)
EFI_TREE := $(BUILD_EFI) $(addprefix $(BUILD_EFI)/, $(EFI_TREE))
$(EFI_EXEC) $(EFI_SO) $(EFI_OBJECTS): | efi-tree
$(BUILD_OVMF_VARS) $(DISK): | efi-tree

TEST_DIRS := test
TEST_SOURCES := $(shell find $(TEST_DIRS) -name "*.c")
TEST_OBJECTS := $(addprefix $(BUILD_TEST)/, $(TEST_SOURCES:%.c=%.o))
TEST_EXECS := $(addprefix $(BUILD_TEST)/, readelf small-exec introspect)
TEST_TREE := $(shell find $(TEST_DIRS) -type d)
TEST_TREE := $(BUILD_TEST) $(addprefix $(BUILD_TEST)/, $(TEST_TREE))
$(TEST_EXECS) $(TEST_OBJECTS): | test-tree

SOURCES := $(KERNEL_SOURCES) $(EFI_SOURCES) $(TEST_SOURCES)
OBJECTS := $(KERNEL_OBJECTS) $(EFI_OBJECTS) $(TEST_OBJECTS)
BUILD_TREE += $(KERNEL_TREE) $(EFI_TREE) $(TEST_TREE)
DEPS := $(OBJECTS:%.o=%.d)

$(OBJECTS): CFLAGS += $(OBJECT_CFLAGS)

.SUFFIXES:

-include $(DEPS)

$(KERNEL): $(KERNEL_LDSCRIPT) $(KERNEL_OBJECTS)
	$(KERNEL_CC) $(CFLAGS) $(KERNEL_CFLAGS) $(KERNEL_LDFLAGS) -o $@ \
		$(KERNEL_OBJECTS) $(KERNEL_LDLIBS)

$(BUILD_KERNEL)/%.o: %.s
	$(KERNEL_AS) -o $@ $<

$(BUILD_KERNEL)/%.o: %.c
	$(KERNEL_CC) $(CPPFLAGS) $(CFLAGS) $(KERNEL_CFLAGS) -c -o $@ $<

$(EFI_EXEC): $(EFI_SO)
	objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel \
		-j .rela -j .rel.* -j .rela.* -j .rel* -j .rela* -j .reloc \
		--target=efi-app-x86_64 $< $@

$(EFI_SO): $(EFI_CRT) $(EFI_LDSCRIPT) $(EFI_OBJECTS)
	$(CC) $(EFI_LDFLAGS) -o $@ $(EFI_CRT) $(EFI_OBJECTS) $(EFI_LDLIBS)

$(BUILD_EFI)/%.o: %.c
	$(CC) $(CPPFLAGS) $(EFI_CPPFLAGS) $(CFLAGS) $(EFI_CFLAGS) -c -o $@ $<

$(BUILD_OVMF_VARS): $(OVMF_VARS)
	install -m 644 $< $@

$(DISK): $(EFI_EXEC) $(KERNEL) efi/startup.nsh
	dd if=/dev/zero of=$@ bs=512 count=93750 status=none
	sgdisk --new 1:0:0 --typecode 1:ef00 \
		--change-name 1:"EFI system partition" $@
	sudo losetup --offset 1048576 --sizelimit 46934528 /dev/loop0 $@
	sudo mkdosfs -F 32 /dev/loop0
	sudo mount /dev/loop0 /mnt
	sudo cp $^ /mnt
	sudo umount /mnt
	sudo losetup -d /dev/loop0

$(BUILD_TEST)/readelf: $(addprefix $(BUILD_KERNEL)/, lib/readelf.o)
$(BUILD_TEST)/readelf: $(addprefix $(BUILD_TEST)/, test/glibc-readelf.o \
		test/readelf.o test/readelf-main.o)
	$(CC) $(CFLAGS) $(TEST_CFLAGS) $(TEST_LDFLAGS) -o $@ $^

$(BUILD_TEST)/%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(TEST_CFLAGS) -c -o $@ $<

$(BUILD_TEST)/small-exec: test/small-exec.S
	$(CPP) $< | $(AS) -o $@.o -
	$(LD) -nostdlib -o $@ $@.o
	-$(RM) $@.o

$(BUILD_TEST)/introspect: $(addprefix $(BUILD_KERNEL)/, lib/elf.o)
$(BUILD_TEST)/introspect: $(addprefix $(BUILD_TEST)/, test/glibc-readelf.o \
		test/readelf.o test/introspect.o)
	$(CC) $(CFLAGS) $(TEST_CFLAGS) $(TEST_LDFLAGS) -o $@ $^

tags: $(SOURCES) $(shell find . -name "*.h" -not -path "./cross/*")
	ctags --exclude=cross/\* --exclude=\*.json --exclude=Makefile -R .

.PHONY: all kernel-tree efi-tree test-tree kernel efi tests clean qemu $(FORCE)

all: tests kernel efi

kernel-tree:
	@mkdir -p $(KERNEL_TREE)

efi-tree:
	@mkdir -p $(EFI_TREE)

test-tree:
	@mkdir -p $(TEST_TREE)

kernel: $(KERNEL)

efi: $(EFI_EXEC)

tests: $(TEST_EXECS)

# remove files, then do a post-order removal of the build tree
clean:
	@-$(RM) $(OBJECTS) $(DEPS) $(KERNEL) $(EFI_SO) $(EFI_EXEC) $(TEST_EXECS) \
		$(BUILD_OVMF_VARS) $(DISK) vgcore.* perf.*
	@for f in $(shell echo $(BUILD_TREE) | tr ' ' '\n' | sort -r); do \
		rmdir $$f 1>/dev/null 2>&1 || true; \
	done

qemu: $(BUILD_OVMF_VARS) $(DISK)
	qemu-system-x86_64 \
		-cpu qemu64 \
		-drive if=pflash,format=raw,unit=0,file=$(OVMF_CODE),readonly=on \
		-drive if=pflash,format=raw,unit=1,file=$(BUILD_OVMF_VARS) \
		-drive file=$(DISK),if=ide,format=raw \
		-net none \
		&
