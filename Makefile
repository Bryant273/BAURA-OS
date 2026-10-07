CROSS_COMPILE ?= i686-elf-
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
NASM ?= nasm
GRUB_FILE ?= grub-file
GRUB_MKRESCUE ?= grub-mkrescue
HOST_CC ?= cc
HOST_CXX ?= c++
HOST_CMAKE ?= cmake
PYTHON ?= python3
X86_64_CROSS_COMPILE ?= x86_64-elf-
X86_64_CC := $(X86_64_CROSS_COMPILE)gcc
X86_64_AR := $(X86_64_CROSS_COMPILE)ar
BUILD_DIR := build
ISO_ROOT := $(BUILD_DIR)/iso
SEL4_SOURCE_DIR := KERNEL/L4/sel4
SEL4_X86_64_BUILD_DIR := $(BUILD_DIR)/sel4-x86_64
SEL4_STAGE_DIR := $(SEL4_X86_64_BUILD_DIR)/stage
SEL4_INITIAL_TASK_ELF := $(BUILD_DIR)/x86_64/baura_init.elf
SEL4_ISO_ROOT := $(BUILD_DIR)/sel4-iso
SEL4_ISO := $(BUILD_DIR)/baura-sel4-x86_64.iso
SEL4_CMAKE_ARGS ?=
QEMU_X86_64 ?= qemu-system-x86_64
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
    BOOT/FIRMWARE/UART/uart_driver.c \
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
    KERNEL/BAURA_CORE/STORAGE/VFS/BAURA_VFS.c \
    KERNEL/BAURA_CORE/SYSTEM_API/baura_system_api.c
CORE_OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(CORE_SOURCES))
CORE_TEST_SOURCES := $(filter-out KERNEL/BAURA_CORE/kernel_main.c KERNEL/BAURA_CORE/runtime.c,$(CORE_SOURCES))
X86_64_CORE_SOURCES := $(filter-out KERNEL/BAURA_CORE/kernel_main.c,$(CORE_SOURCES)) \
    KERNEL/BAURA_CORE/runtime.c \
    KERNEL/BAURA_CORE/SYSTEM_API/baura_ipc_v2.c \
    KERNEL/BAURA_CORE/SYSTEM_API/baura_core_entry.c \
    KERNEL/BAURA_CORE/SYSTEM_API/baura_uart_console.c
X86_64_CORE_OBJECTS := $(patsubst %.c,$(BUILD_DIR)/x86_64/%.o,$(X86_64_CORE_SOURCES))
CORE_TEST_SOURCES += KERNEL/BAURA_CORE/SYSTEM_API/baura_ipc_v2.c \
    KERNEL/BAURA_CORE/SYSTEM_API/baura_core_entry.c \
    KERNEL/BAURA_CORE/SYSTEM_API/baura_uart_console.c
X86_64_CFLAGS ?= -std=gnu11 -m64 -ffreestanding -fno-pic -fno-pie -fno-stack-protector -fno-builtin -Wall -Wextra -Werror -O2 -DBAURA_TARGET_X86_64=1

.PHONY: all legacy32 legacy32-iso run-legacy32 iso run \
	test-core test-core-x86_64 test-abi test-rust \
	test-boot test-communication test-gui test-integration \
	gui-preview sel4-source-check \
	validate-all \
	baura-core-x86_64 sel4-configure-x86_64 sel4-kernel-x86_64 \
	sel4-initial-task-x86_64 sel4-iso-x86_64 run-sel4-x86_64 \
	baura-x86_64 clean

all: baura-x86_64

legacy32: baura_os.elf

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

sel4-source-check:
	$(PYTHON) tools/check_sel4_source.py $(SEL4_SOURCE_DIR)

sel4-configure-x86_64: sel4-source-check
	$(HOST_CMAKE) -S $(SEL4_SOURCE_DIR) -B $(SEL4_X86_64_BUILD_DIR) \
		-DKernelPlatform=pc99 -DKernelSel4Arch=x86_64 \
		-DKernelVerificationBuild=OFF -DKernelPrinting=ON \
		$(SEL4_CMAKE_ARGS)

sel4-kernel-x86_64: sel4-configure-x86_64
	mkdir -p $(SEL4_X86_64_BUILD_DIR)/gen_headers/arch/api \
		$(SEL4_X86_64_BUILD_DIR)/gen_headers/api \
		$(SEL4_X86_64_BUILD_DIR)/generated \
		$(SEL4_X86_64_BUILD_DIR)/generated_prune/plat_mode/machine \
		$(SEL4_X86_64_BUILD_DIR)/generated_prune/arch/object \
		$(SEL4_X86_64_BUILD_DIR)/generated_prune/sel4
	$(HOST_CMAKE) --build $(SEL4_X86_64_BUILD_DIR) --target linker_ld_wrapper
	$(HOST_CMAKE) --build $(SEL4_X86_64_BUILD_DIR) --target kernel.elf
	$(HOST_CMAKE) --build $(SEL4_X86_64_BUILD_DIR) --target sel4_generated
	$(HOST_CMAKE) --install $(SEL4_X86_64_BUILD_DIR) --prefix $(SEL4_STAGE_DIR)

sel4-initial-task-x86_64: baura-core-x86_64 sel4-kernel-x86_64 $(SEL4_INITIAL_TASK_ELF)

$(SEL4_INITIAL_TASK_ELF): BOOT/SEL4/initial_task.c \
		BOOT/SEL4/initial_task_start.S BOOT/SEL4/initial_task.ld \
		$(BUILD_DIR)/x86_64/libbaura_core.a
	mkdir -p $(@D)
	$(X86_64_CC) $(X86_64_CFLAGS) -I$(SEL4_STAGE_DIR)/libsel4/include \
		-c BOOT/SEL4/initial_task.c -o $(BUILD_DIR)/x86_64/baura_init.o
	$(X86_64_CC) -m64 -c BOOT/SEL4/initial_task_start.S \
		-o $(BUILD_DIR)/x86_64/baura_init_start.o
	$(X86_64_CC) -m64 -nostdlib -static -no-pie \
		-Wl,-T,BOOT/SEL4/initial_task.ld -Wl,--build-id=none \
		-Wl,-z,noexecstack -o $@ \
		$(BUILD_DIR)/x86_64/baura_init_start.o \
		$(BUILD_DIR)/x86_64/baura_init.o \
		$(BUILD_DIR)/x86_64/libbaura_core.a

$(SEL4_INITIAL_TASK_ELF): | sel4-kernel-x86_64

baura-x86_64: baura-core-x86_64 sel4-kernel-x86_64 sel4-initial-task-x86_64

sel4-iso-x86_64: $(SEL4_ISO)

$(SEL4_ISO): sel4-kernel-x86_64 $(SEL4_INITIAL_TASK_ELF) BOOT/SEL4/grub.cfg
	mkdir -p $(SEL4_ISO_ROOT)/boot/grub
	cp $(SEL4_X86_64_BUILD_DIR)/kernel.elf $(SEL4_ISO_ROOT)/boot/kernel.elf
	cp $(SEL4_INITIAL_TASK_ELF) $(SEL4_ISO_ROOT)/boot/baura_init.elf
	cp BOOT/SEL4/grub.cfg $(SEL4_ISO_ROOT)/boot/grub/grub.cfg
	$(GRUB_FILE) --is-x86-multiboot2 $(SEL4_ISO_ROOT)/boot/kernel.elf
	$(GRUB_MKRESCUE) -o $@ $(SEL4_ISO_ROOT)

run-sel4-x86_64: $(SEL4_ISO)
	$(QEMU_X86_64) -m 2G -cdrom $(SEL4_ISO) -serial stdio -no-reboot -no-shutdown

legacy32-iso: baura_os.iso

baura_os.iso: baura_os.elf grub.cfg
	mkdir -p $(ISO_ROOT)/boot/grub
	cp baura_os.elf $(ISO_ROOT)/boot/
	cp grub.cfg $(ISO_ROOT)/boot/grub/
	$(GRUB_MKRESCUE) -o $@ $(ISO_ROOT)

run-legacy32: baura_os.iso
	qemu-system-i386 -cdrom baura_os.iso -serial stdio -no-reboot -no-shutdown

iso: legacy32-iso

run: run-legacy32

test-core: test-abi
	mkdir -p $(BUILD_DIR)
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -I. -o $(BUILD_DIR)/core_services_test KERNEL/BAURA_CORE/tests/core_services_test.c $(CORE_TEST_SOURCES)
	$(BUILD_DIR)/core_services_test

test-abi:
	mkdir -p $(BUILD_DIR)
	$(PYTHON) KERNEL/BAURA_CORE/SYSTEM_API/check_abi_v2.py
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -I. -o $(BUILD_DIR)/baura_ipc_v2_test KERNEL/BAURA_CORE/tests/baura_ipc_v2_test.c KERNEL/BAURA_CORE/SYSTEM_API/baura_ipc_v2.c
	$(BUILD_DIR)/baura_ipc_v2_test

test-boot:
	mkdir -p $(BUILD_DIR)
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -IBOOT/BOOTLOADER \
		-o $(BUILD_DIR)/boot_handoff_test \
		BOOT/tests/boot_handoff_test.c \
		BOOT/BOOTLOADER/KERNEL_LOADER/L4_LOADER.c
	$(BUILD_DIR)/boot_handoff_test
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror \
		-o $(BUILD_DIR)/uart_driver_test \
		BOOT/tests/uart_driver_test.c BOOT/FIRMWARE/UART/uart_driver.c \
		KERNEL/BAURA_CORE/SYSTEM_API/baura_uart_console.c
	$(BUILD_DIR)/uart_driver_test
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror \
		-o $(BUILD_DIR)/uefi_memory_map_test \
		BOOT/tests/uefi_memory_map_test.c BOOT/FIRMWARE/UEFI/uefi_mem.c
	$(BUILD_DIR)/uefi_memory_map_test

test-communication:
	mkdir -p $(BUILD_DIR)
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror \
		-o $(BUILD_DIR)/rpc_core_test \
		COMMUNICATION/RPC/tests/rpc_core_test.c \
		COMMUNICATION/RPC/BAURA_RPC_CORE.c
	$(BUILD_DIR)/rpc_core_test
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror \
		-o $(BUILD_DIR)/message_bus_test \
		COMMUNICATION/MESSAGE_BUS/tests/message_bus_test.c \
		COMMUNICATION/MESSAGE_BUS/BAURA_MESSAGE_BUS.c
	$(BUILD_DIR)/message_bus_test

test-gui:
	mkdir -p $(BUILD_DIR)
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -I. -c \
		-o $(BUILD_DIR)/baura_service_manager_gui_test.o \
		KERNEL/BAURA_CORE/SERVICE_MANAGER/baura_service_manager.c
	$(HOST_CXX) -std=c++17 -Wall -Wextra -Werror -IGUI \
		-o $(BUILD_DIR)/gui_render_test \
		GUI/tests/gui_render_test.cpp GUI/BAURA_GUI_FFI.cpp \
		GUI/INPUT/ps2_driver.cpp \
		GUI/PLATFORM/baura_gui_platform.cpp \
		GUI/PLATFORM/baura_gui_core_service.cpp \
		GUI/DISPLAY_SERVER/BAURA_DISPLAY_SERVER.cpp \
		GUI/COMPOSITOR/BAURA_COMPOSITOR.cpp \
		GUI/WINDOW_MANAGER/BAURA_WINDOW_MANAGER.cpp \
		GUI/WINDOW_MANAGER/input_handler.cpp \
		GUI/BGE/renderer.cpp GUI/BGE/renderer_core.cpp \
		GUI/BGE/effects_engine.cpp GUI/BGE/gpu_accel_3d.cpp \
		GUI/BGE/window_manager.cpp \
		GUI/BGE/ui_framework.cpp \
		GUI/UI_FRAMEWORK/baura_ui_toolkit.cpp \
		GUI/UI_FRAMEWORK/baura_hybrid_taskbar.cpp \
		GUI/UI_FRAMEWORK/papirus_icon_catalog.cpp \
		GUI/UI_FRAMEWORK/system_bubble.cpp \
		GUI/UI_FRAMEWORK/ui_extensions.cpp \
		GUI/DESKTOP/baura_desktop.cpp \
		GUI/LOGIN_SCREEN/login_screen.cpp \
		BOOT/INSTALLER/installer_gui.cpp \
		$(BUILD_DIR)/baura_service_manager_gui_test.o
	$(BUILD_DIR)/gui_render_test

gui-preview: test-gui
	BAURA_GUI_PREVIEW_PATH=$(BUILD_DIR)/baura-gui-preview.bmp \
		$(BUILD_DIR)/gui_render_test

test-integration: test-boot test-communication test-gui test-core

validate-all: test-integration test-rust test-core-x86_64 baura-x86_64

test-core-x86_64: test-abi
	mkdir -p $(BUILD_DIR)/x86_64
	$(HOST_CC) -m64 -std=c11 -Wall -Wextra -Werror -DBAURA_TARGET_X86_64=1 -I. -o $(BUILD_DIR)/x86_64/core_services_test KERNEL/BAURA_CORE/tests/core_services_test.c $(CORE_TEST_SOURCES)
	$(BUILD_DIR)/x86_64/core_services_test

test-rust:
	cargo test --manifest-path KERNEL/BAURA_CORE/STORAGE/VFS/Cargo.toml
	cargo test --manifest-path KERNEL/BAURA_CORE/SECURITY/Cargo.toml
	cargo test --manifest-path KERNEL/BAURA_CORE/DRIVER_MANAGER/Cargo.toml

clean:
	rm -rf $(BUILD_DIR) baura_os.elf baura_os.iso
