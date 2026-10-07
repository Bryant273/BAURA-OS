# BOOT layer status

`boot.asm` and the root GRUB configuration implement the legacy 32-bit
Multiboot bring-up path. They do not start the x86_64 seL4 build. The default
`make` target now builds the separate x86_64 Core archive and seL4 kernel;
it does not create an integrated image or launch a BAURA initial task.

`BOOT/BOOTLOADER/KERNEL_HANDOFF.h` defines a versioned, kernel-neutral handoff
record. `L4_LOADER.c` validates its structure and delegates platform-specific
payload validation and control transfer to required callbacks. It does not
load a kernel image or jump into seL4 by itself. The transfer callback must be
implemented only after the selected image format and loader protocol are
confirmed.

The 16550-compatible UART driver operates on an injected port-I/O backend and
uses bounded transmit polling. The current 32-bit Multiboot bring-up kernel
installs privileged x86 port-I/O callbacks for COM1 (`0x3F8`) and uses this
driver for its boot and Core-ready messages; this is intended for the
QEMU-emulated PC serial port and compatible bare-metal legacy boot only. It
does not grant port access under seL4. `baura_core_uart_console_write` adapts
the same driver to BAURA Core's callback interface when a platform has
initialized it with authorized I/O operations.

With the legacy cross-toolchain, `make run-legacy32` launches the Multiboot
image in QEMU with the serial console attached to stdio. The expected output
includes `BAURA-OS booting` followed by
`CORE_READY: BAURA Core services initialized`. This checks the legacy QEMU
serial path only; it is not a seL4 boot test. `make run` remains a compatibility
alias for this legacy target.

`FIRMWARE/UEFI/uefi_mem.c` validates a bounded, provider-normalized memory-map
snapshot (region bounds, types, capacity, and overlap). It is not itself an
EFI protocol binding; a UEFI adapter must convert the firmware's descriptors
and preserve their meaning before calling it. The snapshot type is BAURA's
normalized contract, not the native UEFI descriptor layout.

`INITRD/manifest.txt` is a plan, not evidence that these binaries are built or
present in a boot image.

`GUI/UI_FRAMEWORK/baura_brand.h` contains a framebuffer-based BAURA startup
splash and the shared software-rendered mark. The splash is currently a
reusable GUI surface only: the legacy Multiboot path has no connected graphics
framebuffer, and the seL4 path has no integrated initial task or display
mapping. It is not shown during a real boot yet.

Host checks: `make test-boot`.
