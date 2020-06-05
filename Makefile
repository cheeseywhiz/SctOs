BUILD ?= ./build
BUILD_EFI ?= $(BUILD)/efi
BUILD_TREE += $(BUILD)

# Dependencies:
OVMF_CODE := /usr/share/edk2-ovmf/x64/OVMF_CODE.fd
OVMF_VARS := /usr/share/edk2-ovmf/x64/OVMF_VARS.fd
EFI_CRT := $(HOME)/.local/lib/crt0-efi-x86_64.o
EFI_LDSCRIPT := $(HOME)/.local/lib/elf_x86_64_efi.lds
EFI_INCLUDE := $(HOME)/.local/include/efi

BUILD_OVMF_VARS := $(BUILD_EFI)/OVMF_VARS.fd
DISK := $(BUILD)/disk.img

# default rule needs to be pretty high up
.PHONY: all
all: efi disk

CFLAGS += -std=gnu11 -O2 -fdiagnostics-color=always
CPPFLAGS += -MMD -MP
CFLAGS += -Werror -Wall -Wextra -pedantic -Wshadow -Wpointer-arith \
	-Wcast-align -Wwrite-strings -Wmissing-prototypes -Wmissing-declarations \
	-Wredundant-decls -Wnested-externs -Winline -Wno-long-long -Wconversion \
	-Wstrict-prototypes
EFI_CPPFLAGS += -I$(EFI_INCLUDE) -I$(EFI_INCLUDE)/x86_64 -DGNU_EFI_USE_MS_ABI
EFI_CFLAGS += -mno-red-zone -mno-avx -fshort-wchar -fno-strict-aliasing \
	-ffreestanding -fno-stack-protector -fno-merge-constants -fPIC \
	-Wno-write-strings -Wno-redundant-decls -Wno-strict-prototypes
ifneq ($(EFI_DEBUG),)
EFI_CPPFLAGS += -D_EFI_DEBUG=$(EFI_DEBUG)
EFI_CFLAGS += -O0
#EFI_CFLAGS += -Og -fno-inline -Wno-error=inline
EFI_CFLAGS +=-g3
endif
EFI_LDFLAGS += -nostdlib -shared -T $(EFI_LDSCRIPT) -L$(HOME)/.local/lib \
	-Wl,-Bsymbolic,--warn-common,--defsym=EFI_SUBSYSTEM=0xa,--no-undefined \
	-Wl,--fatal-warnings,--build-id=sha1,-z,nocombreloc
EFI_LDLIBS := -lefi -lgnuefi -lgcc

QEMUDEPS := efi $(BUILD_OVMF_VARS) $(DISK)
QEMUFLAGS += \
	-drive if=pflash,format=raw,unit=0,file=$(OVMF_CODE),readonly=on \
	-drive if=pflash,format=raw,unit=1,file=$(BUILD_OVMF_VARS) \
	-drive file=$(DISK),if=ide,format=raw \
	-net none \
	-nographic \
	-serial mon:stdio \

ifneq ($(EFI_DEBUG),)
QEMUFLAGS += -s -S
endif

ifneq ($(NOKVM),)
QEMUFLAGS += -cpu qemu64
else
QEMUFLAGS += -enable-kvm -cpu host
endif

EFI_SOURCES := efi/efi_main.c
EFI_OBJECTS := $(BUILD_EFI)/efi/efi_main.o
EFI_SO := $(BUILD_EFI)/opsys-loader.so
EFI_EXEC := $(BUILD_EFI)/opsys-loader.efi
EFI_DEBUG_EXEC := $(BUILD_EFI)/opsys-loader-debug.efi
EFI_EXECS := $(EFI_SO) $(EFI_EXEC) $(EFI_DEBUG_EXEC)
EFI_TREE := $(BUILD_EFI)/efi
$(EFI_EXECS) $(EFI_OBJECTS): | efi-tree

SOURCES := $(EFI_SOURCES)
OBJECTS := $(EFI_OBJECTS)
BUILD_TREE += $(EFI_TREE)
DEPS := $(OBJECTS:%.o=%.d)

.SUFFIXES:

-include $(DEPS)

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

$(BUILD_OVMF_VARS): $(OVMF_VARS) | efi-tree
	install -m 644 $< $@

$(DISK): $(EFI_EXEC) efi/startup.nsh | $(BUILD)
	dd if=/dev/zero of=$@ bs=512 count=93750 status=none
	sgdisk --new 1:0:0 --typecode 1:ef00 \
		--change-name 1:"EFI system partition" $@
	sudo losetup --offset 1048576 --sizelimit 46934528 /dev/loop0 $@
	sudo mkdosfs -F 32 /dev/loop0
	sudo mount /dev/loop0 /mnt
	sudo cp $^ /mnt
	sudo umount /mnt
	sudo losetup -d /dev/loop0

.PHONY: efi-tree efi disk clean compile_commands.json qemu-deps qemu \
	print-efi-execs $(FORCE)

efi-tree:
	@mkdir -p $(EFI_TREE)

ifneq ($(EFI_DEBUG),)
efi: $(EFI_EXEC) $(EFI_DEBUG_EXEC)
else
efi: $(EFI_EXEC)
endif

disk: $(DISK)

# remove files, then do a post-order removal of the build tree
clean:
	@-$(RM) $(OBJECTS) $(DEPS) $(EFI_EXECS) $(BUILD_OVMF_VARS) $(DISK)
	@for f in $(shell echo $(BUILD_TREE) | tr ' ' '\n' | sort -r); do \
		rmdir $$f 1>/dev/null 2>&1 || true; \
	done

compile_commands.json:
	make clean
	$(RM) $@
	bear make all -j5

qemu-deps: $(QEMUDEPS)

qemu: qemu-deps
	qemu-system-x86_64 $(QEMUFLAGS)

# pass this information to gdb.py
print-efi-execs:
	@echo $(EFI_EXEC)
	@echo $(EFI_DEBUG_EXEC)
