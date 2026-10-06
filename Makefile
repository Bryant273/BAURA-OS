CROSS_COMPILE ?= i686-elf-
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
NASM ?= nasm
GRUB_FILE ?= grub-file
GRUB_MKRESCUE ?= grub-mkrescue
HOST_CC ?= cc
HOST_CMAKE ?= cmake
X86_64_CROSS_COMPILE ?= x86_64-elf-
X86_64_CC := $(X86_64_CROSS_COMPILE)gcc
X86_64_AR := $(X86_64_CROSS_COMPILE)ar
BUILD_DIR := build
ISO_ROOT := $(BUILD_DIR)/iso
SEL4_SOURCE_DIR := KERNEL/L4/sel4
SEL4_X86_64_BUILD_DIR := $(BUILD_DIR)/sel4-x86_64
SEL4_CMAKE_ARGS ?=
CFLAGS := -std=gnu11 -m32 -ffreestanding -fno-pic -fno-pie -fno-stack-protector -fno-builtin -Wall -Wextra -Werror -O2
LDFLAGS := -m elf_i386 -T linker.ld -nostdlib

CORE_SOURCES := \
    KERNEL/BAURA_CORE/kernel_main.c \
    KERNEL/BAURA_CORE/runtime.c \
    KERNEL/BAURA_CORE/SYSTEM_MANAGER/baura_system_manager.c \
    KERNEL/BAURA_CORE/HARDWARE_MANAGER/baura_hardware_manager.c \
    KERNEL/BAURA_CORE/UPDATE_MANAGER/baura_update_manager.c \
    KERNEL/BAURA_CORE/VIRTUALIZATION/baura_virtualization.c \
    KERNEL/BAURA_CORE/CONFIGURATION/baura_configuration.c \
    KERNEL/BAURA_CORE/DEVICE_MANAGER/BAURA_DEVICE_MGR.c \
    KERNEL/BAURA_CORE/DEVICE_MANAGER/baura_device_manager.c \
    KERNEL/BAURA_CORE/DRIVER_MANAGER/baura_driver_manager.c \
    KERNEL/BAURA_CORE/EVENT_BUS/baura_event_bus.c \
    KERNEL/BAURA_CORE/FAULT_ENGINE/baura_fault_engine.c \
    KERNEL/BAURA_CORE/RECOVERY_ENGINE/baura_recovery_engine.c \
    KERNEL/BAURA_CORE/LOGGING/baura_logging.c \
    KERNEL/BAURA_CORE/MONITORING/baura_monitoring.c \
    KERNEL/BAURA_CORE/NETWORK/baura_network.c \
    KERNEL/BAURA_CORE/POWER_MANAGER/baura_power_manager.c \
    KERNEL/BAURA_CORE/PERFORMANCE/baura_performance.c \
    KERNEL/BAURA_CORE/PROCESS_MANAGER/baura_process_manager.c \
    KERNEL/BAURA_CORE/RESOURCE_MANAGER/baura_resource_manager.c \
    KERNEL/BAURA_CORE/SECURITY/baura_security.c \
    KERNEL/BAURA_CORE/SERVICE_MANAGER/baura_service_manager.c \
    KERNEL/BAURA_CORE/STORAGE/baura_storage.c \
    KERNEL/BAURA_CORE/STORAGE/VOLUME_MANAGER/volume_manager.c \
    KERNEL/BAURA_CORE/SYSTEM_API/baura_system_api.c
CORE_OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(CORE_SOURCES))
CORE_TEST_SOURCES := $(filter-out KERNEL/BAURA_CORE/kernel_main.c KERNEL/BAURA_CORE/runtime.c,$(CORE_SOURCES))
X86_64_CORE_SOURCES := $(filter-out KERNEL/BAURA_CORE/kernel_main.c,$(CORE_SOURCES)) \
    KERNEL/BAURA_CORE/runtime.c \
    KERNEL/BAURA_CORE/SYSTEM_API/baura_ipc_v2.c \
    KERNEL/BAURA_CORE/SYSTEM_API/baura_core_entry.c
X86_64_CORE_OBJECTS := $(patsubst %.c,$(BUILD_DIR)/x86_64/%.o,$(X86_64_CORE_SOURCES))
CORE_TEST_SOURCES += KERNEL/BAURA_CORE/SYSTEM_API/baura_ipc_v2.c \
    KERNEL/BAURA_CORE/SYSTEM_API/baura_core_entry.c
X86_64_CFLAGS ?= -std=gnu11 -m64 -ffreestanding -fno-pic -fno-pie -fno-stack-protector -fno-builtin -Wall -Wextra -Werror -O2 -DBAURA_TARGET_X86_64=1

.PHONY: all iso run test-core test-core-x86_64 test-abi test-rust \
	baura-core-x86_64 sel4-configure-x86_64 sel4-kernel-x86_64 \
	baura-x86_64 clean

all: baura_os.elf

baura_os.elf: $(BUILD_DIR)/boot.o $(CORE_OBJECTS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(BUILD_DIR)/boot.o $(CORE_OBJECTS)
	$(GRUB_FILE) --is-x86-multiboot $@

$(BUILD_DIR)/boot.o: BOOT/boot.asm
	mkdir -p $(@D)
	$(NASM) -f elf32 $< -o $@

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/x86_64/%.o: %.c
	mkdir -p $(@D)
	$(X86_64_CC) $(X86_64_CFLAGS) -c $< -o $@

baura-core-x86_64: $(BUILD_DIR)/x86_64/libbaura_core.a

$(BUILD_DIR)/x86_64/libbaura_core.a: $(X86_64_CORE_OBJECTS)
	$(X86_64_AR) rcs $@ $^

sel4-configure-x86_64:
	$(HOST_CMAKE) -S $(SEL4_SOURCE_DIR) -B $(SEL4_X86_64_BUILD_DIR) \
		-DPLATFORM=pc99 -DKernelSel4Arch=x86_64 -DKernelPrinting=ON \
		$(SEL4_CMAKE_ARGS)

sel4-kernel-x86_64: sel4-configure-x86_64
	$(HOST_CMAKE) --build $(SEL4_X86_64_BUILD_DIR) --target kernel.elf

baura-x86_64: baura-core-x86_64 sel4-kernel-x86_64

iso: baura_os.iso

baura_os.iso: baura_os.elf grub.cfg
	mkdir -p $(ISO_ROOT)/boot/grub
	cp baura_os.elf $(ISO_ROOT)/boot/
	cp grub.cfg $(ISO_ROOT)/boot/grub/
	$(GRUB_MKRESCUE) -o $@ $(ISO_ROOT)

run: baura_os.iso
	qemu-system-i386 -cdrom baura_os.iso -serial stdio -no-reboot -no-shutdown

test-core: test-abi
	mkdir -p $(BUILD_DIR)
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -I. -o $(BUILD_DIR)/core_services_test KERNEL/BAURA_CORE/tests/core_services_test.c $(CORE_TEST_SOURCES)
	$(BUILD_DIR)/core_services_test

test-abi:
	mkdir -p $(BUILD_DIR)
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -I. -o $(BUILD_DIR)/baura_ipc_v2_test KERNEL/BAURA_CORE/tests/baura_ipc_v2_test.c KERNEL/BAURA_CORE/SYSTEM_API/baura_ipc_v2.c
	$(BUILD_DIR)/baura_ipc_v2_test

test-core-x86_64: test-abi
	mkdir -p $(BUILD_DIR)/x86_64
	$(HOST_CC) -m64 -std=c11 -Wall -Wextra -Werror -DBAURA_TARGET_X86_64=1 -I. -o $(BUILD_DIR)/x86_64/core_services_test KERNEL/BAURA_CORE/tests/core_services_test.c $(CORE_TEST_SOURCES)
	$(BUILD_DIR)/x86_64/core_services_test

test-rust:
	cargo test --manifest-path KERNEL/BAURA_CORE/STORAGE/VFS/Cargo.toml
	cargo test --manifest-path KERNEL/BAURA_CORE/SECURITY/Cargo.toml

clean:
	rm -rf $(BUILD_DIR) baura_os.elf baura_os.iso
