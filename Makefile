# test build is in tree, kernel and efi loader are out of tree
BUILD_KERNEL ?= ./build/kernel
BUILD_EFI ?= ./build/efi
BUILD_TEST ?= .
BUILD_TREE += ./build

OVMF_CODE := /usr/share/edk2-ovmf/x64/OVMF_CODE.fd
OVMF_VARS := /usr/share/edk2-ovmf/x64/OVMF_VARS.fd
BUILD_OVMF_VARS := $(BUILD_EFI)/OVMF_VARS.fd
DISK := $(BUILD_EFI)/disk.img

# default rule needs to be pretty high up
.PHONY: disk
disk: $(DISK)

KERNEL_CC := ./cross/bin/x86_64-elf-gcc

CPPFLAGS += -MMD -MP -iquote include -I$(HOME)/.local/include/efi \
	-I$(HOME)/.local/include/efi/x86_64 -DGNU_EFI_USE_MS_ABI
CFLAGS += -std=gnu11 -O2 -fdiagnostics-color=always -fno-common
CFLAGS += -Werror -Wall -Wextra -pedantic -Wshadow -Wpointer-arith \
	-Wcast-align -Wwrite-strings -Wmissing-prototypes -Wmissing-declarations \
	-Wredundant-decls -Wnested-externs -Winline -Wno-long-long -Wconversion \
	-Wstrict-prototypes
KERNEL_CFLAGS += -ffreestanding -fPIE -mno-red-zone -mno-mmx -mno-sse -mno-sse2
KERNEL_CPPFLAGS += -D_KERNEL
ifneq ($(KERNEL_DEBUG),)
KERNEL_CPPFLAGS += -D_KERNEL_DEBUG=$(KERNEL_DEBUG)
KERNEL_CFLAGS += -O0 -g3
KERNEL_ASFLAGS += -g3
endif
KERNEL_LDFLAGS += -nostdlib -static-pie -Wl,-static,-pie,--no-dynamic-linker \
	-Wl,-z,separate-code,-z,max-page-size=0x1000,-z,noexecstack,-z,relro \
	-Wl,-e,kernel_main
KERNEL_LDLIBS := -lgcc
# TODO: patch gnu-efi to remove these -Wno- flags
EFI_CFLAGS += -mno-red-zone -mno-avx -fshort-wchar -fno-strict-aliasing \
	-ffreestanding -fno-stack-protector -fno-merge-constants -fPIC \
	-Wno-write-strings -Wno-redundant-decls -Wno-strict-prototypes
ifneq ($(EFI_DEBUG),)
EFI_CPPFLAGS += -D_EFI_DEBUG=$(EFI_DEBUG)
EFI_CFLAGS += -O0 -g3
EFI_ASFLAGS += -g
endif
EFI_CRT := $(HOME)/.local/lib/crt0-efi-x86_64.o
EFI_LDSCRIPT := $(HOME)/.local/lib/elf_x86_64_efi.lds
EFI_LDFLAGS += -nostdlib -shared -T $(EFI_LDSCRIPT) -L$(HOME)/.local/lib \
	-Wl,-Bsymbolic,--warn-common,--defsym=EFI_SUBSYSTEM=0xa,--no-undefined \
	-Wl,--fatal-warnings,--build-id=sha1,-z,nocombreloc
EFI_LDLIBS := -lefi -lgnuefi -lgcc
TEST_CFLAGS += -O0 -ggdb
TEST_LDFLAGS += -Wl,--hash-style=sysv

QEMUDEPS := $(BUILD_OVMF_VARS) $(DISK) efi
QEMUFLAGS += \
	-drive if=pflash,format=raw,unit=0,file=$(OVMF_CODE),readonly=on \
	-drive if=pflash,format=raw,unit=1,file=$(BUILD_OVMF_VARS) \
	-drive file=$(DISK),if=ide,format=raw \
	-serial file:out.txt \
	-net none \
	-enable-kvm \
	-cpu host \
	-m 1G \
	-nographic \
	-serial mon:stdio \
	-no-reboot \
	-s \

ifneq ($(EFI_DEBUG)$(KERNEL_DEBUG),)
QEMUFLAGS += -S
endif

KERNEL_DIRS := lib src
KERNEL_ASM_GEN := $(BUILD_KERNEL)/gen/vectors.S
KERNEL_GEN := $(KERNEL_ASM_GEN)
KERNEL_ASM_SOURCES := $(shell find $(KERNEL_DIRS) -name "*.S")
KERNEL_C_SOURCES := $(shell find $(KERNEL_DIRS) -name "*.c")
KERNEL_SOURCES := $(KERNEL_ASM_SOURCES) $(KERNEL_C_SOURCES)
KERNEL_SOURCES += $(KERNEL_ASM_GEN)
KERNEL_OBJECTS := $(KERNEL_ASM_SOURCES:%.S=%.o) $(KERNEL_C_SOURCES:%.c=%.o)
KERNEL_OBJECTS := $(addprefix $(BUILD_KERNEL)/, $(KERNEL_OBJECTS))
KERNEL_OBJECTS += $(KERNEL_ASM_GEN:%.S=%.o)
KERNEL_DEPS := $(KERNEL_ASM_SOURCES:%.S=%.d) $(KERNEL_C_SOURCES:%.c=%.d)
KERNEL_DEPS := $(addprefix $(BUILD_KERNEL)/, $(KERNEL_DEPS))
KERNEL_DEPS += $(KERNEL_ASM_GEN:%.S=%.d)
KERNEL := $(BUILD_KERNEL)/opsys
KERNEL_TREE := $(shell find $(KERNEL_DIRS) -type d) gen
KERNEL_TREE := $(BUILD_KERNEL) $(addprefix $(BUILD_KERNEL)/, $(KERNEL_TREE))
$(KERNEL) $(KERNEL_OBJECTS) $(KERNEL_ASM_GEN): | kernel-tree

EFI_C_SOURCES := $(shell find efi -name "*.c") lib/readelf.c lib/opsys/x86.c
EFI_ASM_SOURCES := $(shell find efi -name "*.S")
EFI_SOURCES := $(EFI_C_SOURCES) $(EFI_ASM_SOURCES)
EFI_OBJECTS := $(EFI_C_SOURCES:%.c=%.o) $(EFI_ASM_SOURCES:%.S=%.o)
EFI_OBJECTS := $(addprefix $(BUILD_EFI)/, $(EFI_OBJECTS))
EFI_DEPS := $(EFI_C_SOURCES:%.c=%.d) $(EFI_ASM_SOURCES:%.S=%.d)
EFI_DEPS := $(addprefix $(BUILD_EFI)/, $(EFI_DEPS))
EFI_SO := $(BUILD_EFI)/opsys-loader.so
EFI_EXEC := $(BUILD_EFI)/opsys-loader.efi
EFI_DEBUG_EXEC := $(BUILD_EFI)/opsys-loader-debug.efi
EFI_EXECS := $(EFI_SO) $(EFI_EXEC) $(EFI_DEBUG_EXEC)
EFI_TREE := $(shell find efi -type d) lib lib/opsys
EFI_TREE := $(BUILD_EFI) $(addprefix $(BUILD_EFI)/, $(EFI_TREE))
$(EFI_EXECS) $(EFI_OBJECTS): | efi-tree
$(BUILD_OVMF_VARS) $(DISK): | efi-tree

TEST_DIRS := test
TEST_SOURCES := $(shell find $(TEST_DIRS) -name "*.c")
TEST_OBJECTS := $(addprefix $(BUILD_TEST)/, $(TEST_SOURCES:%.c=%.o))
TEST_DEPS := $(addprefix $(BUILD_TEST)/, $(TEST_SOURCES:%.c=%.d))
TEST_EXECS := $(addprefix $(BUILD_TEST)/, \
	readelf small-exec introspect gen-vectors)
TEST_TREE := $(shell find $(TEST_DIRS) -type d)
TEST_TREE := $(BUILD_TEST) $(addprefix $(BUILD_TEST)/, $(TEST_TREE))
$(TEST_EXECS) $(TEST_OBJECTS): | test-tree

GEN := $(KERNEL_GEN)
SOURCES := $(KERNEL_SOURCES) $(EFI_SOURCES) $(TEST_SOURCES)
OBJECTS := $(KERNEL_OBJECTS) $(EFI_OBJECTS) $(TEST_OBJECTS)
BUILD_TREE += $(KERNEL_TREE) $(EFI_TREE) $(TEST_TREE)
DEPS := $(KERNEL_DEPS) $(EFI_DEPS) $(TEST_DEPS)

.SUFFIXES:

-include $(DEPS)

$(KERNEL): $(KERNEL_LDSCRIPT) $(KERNEL_OBJECTS)
	$(KERNEL_CC) $(CFLAGS) $(KERNEL_CFLAGS) $(KERNEL_LDFLAGS) -o $@ \
		$(KERNEL_OBJECTS) $(KERNEL_LDLIBS)
ifneq ($(KERNEL_DEBUG),)
	@echo kernel debug ready
endif

$(BUILD_KERNEL)/%.o: %.S
	$(KERNEL_CC) $(CPPFLAGS) $(KERNEL_CPPFLAGS) $(KERNEL_ASFLAGS) -c -o $@ $<

$(BUILD_KERNEL)/%.o: %.c
	$(KERNEL_CC) $(CPPFLAGS) $(KERNEL_CPPFLAGS) $(CFLAGS) $(KERNEL_CFLAGS) \
		-c -o $@ $<

$(BUILD_KERNEL)/gen/vectors.S: $(BUILD_TEST)/gen-vectors
	$(BUILD_TEST)/gen-vectors > $@

$(BUILD_KERNEL)/gen/%.o: $(BUILD_KERNEL)/gen/%.S
	$(KERNEL_CC) $(CPPFLAGS) $(KERNEL_CPPFLAGS) $(KERNEL_ASFLAGS) -c -o $@ $<

$(BUILD_KERNEL)/gen/%.o: $(BUILD_KERNEL)/gen/%.c
	$(KERNEL_CC) $(CPPFLAGS) $(KERNEL_CPPFLAGS) $(CFLAGS) $(KERNEL_CFLAGS) \
		-c -o $@ $<

$(EFI_EXEC): $(EFI_SO)
	objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel* \
		--target=efi-app-x86_64 $< $@

$(EFI_DEBUG_EXEC): $(EFI_SO)
	objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel* \
		-j .debug* --target=efi-app-x86_64 $< $@
ifneq ($(EFI_DEBUG),)
	@echo efi debug ready
endif

$(EFI_SO): $(EFI_CRT) $(EFI_LDSCRIPT) $(EFI_OBJECTS)
	$(CC) $(EFI_LDFLAGS) -o $@ $(EFI_CRT) $(EFI_OBJECTS) $(EFI_LDLIBS)

$(BUILD_EFI)/%.o: %.c
	$(CC) $(CPPFLAGS) $(EFI_CPPFLAGS) $(CFLAGS) $(EFI_CFLAGS) -c -o $@ $<

$(BUILD_EFI)/%.o: %.S
	$(CC) $(CPPFLAGS) $(EFI_CPPFLAGS) $(EFI_ASFLAGS) -c -o $@ $<

$(BUILD_OVMF_VARS): $(OVMF_VARS)
	install -m 644 $< $@

# if the disk does not exist, create it
# this allows the partition guid to stay the same across rebuilds
# also, it's faster, because sgdisk is slow
$(DISK): $(EFI_EXEC) $(KERNEL) efi/startup.nsh
ifeq ($(wildcard $(DISK)),)
	dd if=/dev/zero of=$@ bs=512 count=93750 status=none
	sgdisk --new 1:0:0 --typecode 1:ef00 \
		--change-name 1:"EFI system partition" $@
endif
	sudo losetup --offset 1048576 --sizelimit 46934528 /dev/loop0 $@
ifeq ($(wildcard $(DISK)),)
	sudo mkdosfs -F 32 /dev/loop0
endif
	sudo mount /dev/loop0 /mnt
	sudo cp $^ /mnt
	sudo umount /mnt
	sudo losetup -d /dev/loop0

$(BUILD_TEST)/readelf: $(addprefix $(BUILD_KERNEL)/, lib/readelf.o)
$(BUILD_TEST)/readelf: $(addprefix $(BUILD_TEST)/, test/glibc-readelf.o \
		test/print-elf.o test/readelf.o)
	$(CC) $(CFLAGS) $(TEST_CFLAGS) $(TEST_LDFLAGS) -o $@ $^

$(BUILD_TEST)/%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(TEST_CFLAGS) -c -o $@ $<

$(BUILD_TEST)/small-exec: test/small-exec.S
	$(CPP) $< | $(AS) -o $@.o -
	$(LD) -nostdlib -o $@ $@.o
	-$(RM) $@.o

$(BUILD_TEST)/introspect: $(addprefix $(BUILD_KERNEL)/, lib/elf.o)
$(BUILD_TEST)/introspect: $(addprefix $(BUILD_TEST)/, \
		test/glibc-readelf.o test/print-elf.o test/introspect.o)
	$(CC) $(CFLAGS) $(TEST_CFLAGS) $(TEST_LDFLAGS) -o $@ $^

$(BUILD_TEST)/gen-vectors: $(addprefix $(BUILD_TEST)/, test/gen-vectors.o)
	$(CC) $(CFLAGS) $(TEST_CFLAGS) $(TEST_LDFLAGS) -o $@ $^

tags: $(SOURCES) $(shell find . -name "*.h" -not -path "./cross/*")
	ctags --exclude=cross/\* --exclude=\*.json --exclude=Makefile -R .

.PHONY: all kernel-tree efi-tree test-tree kernel efi tests clean \
	compile_commands.json qemu-deps qemu print-debug-execs $(FORCE)

all: tests kernel efi disk

kernel-tree:
	@mkdir -p $(KERNEL_TREE)

efi-tree:
	@mkdir -p $(EFI_TREE)

test-tree:
	@mkdir -p $(TEST_TREE)

kernel: $(KERNEL)

ifneq ($(EFI_DEBUG),)
efi: $(EFI_EXEC) $(EFI_DEBUG_EXEC)
else
efi: $(EFI_EXEC)
endif

tests: $(TEST_EXECS)

# remove files, then do a post-order removal of the build tree
clean:
	@-$(RM) $(OBJECTS) $(GEN) $(DEPS) $(KERNEL) $(EFI_EXECS) $(TEST_EXECS) \
		$(BUILD_OVMF_VARS) $(DISK) vgcore.* perf.*
	@for f in $(shell echo $(BUILD_TREE) | tr ' ' '\n' | sort -r); do \
		rmdir $$f 1>/dev/null 2>&1 || true; \
	done

compile_commands.json:
	make clean
	$(RM) $@
	bear make all -j5

qemu-deps: $(QEMUDEPS)
ifneq ($(EFI_DEBUG)$(KERNEL_DEBUG),)
	@echo qemu ready
endif

qemu: qemu-deps
	qemu-system-x86_64 $(QEMUFLAGS)

# pass this information to gdb.py
print-debug-execs:
	@echo $(EFI_EXEC)
	@echo $(EFI_DEBUG_EXEC)
	@echo $(KERNEL)
