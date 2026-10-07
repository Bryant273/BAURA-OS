# BOOT layer status

`boot.asm` and the root GRUB configuration still implement the legacy 32-bit
Multiboot bring-up path. They do not start the x86_64 seL4 build.

`BOOT/BOOTLOADER/KERNEL_HANDOFF.h` defines a versioned, kernel-neutral handoff
record. `L4_LOADER.c` validates its structure and delegates platform-specific
payload validation and control transfer to required callbacks. It does not
load a kernel image or jump into seL4 by itself. The transfer callback must be
implemented only after the selected image format and loader protocol are
confirmed.

The UART driver is an injected port-I/O backend with bounded transmit polling.
It performs no privileged I/O until the platform supplies read/write callbacks.
`baura_core_uart_console_write` adapts this driver to BAURA Core's console
callback; the platform must initialize the UART backend with authorized I/O
operations before using it.

`FIRMWARE/UEFI/uefi_mem.c` validates a bounded, provider-normalized memory-map
snapshot (region bounds, types, capacity, and overlap). It is not itself an
EFI protocol binding; a UEFI adapter must convert the firmware's descriptors
and preserve their meaning before calling it. The snapshot type is BAURA's
normalized contract, not the native UEFI descriptor layout.

`INITRD/manifest.txt` is a plan, not evidence that these binaries are built or
present in a boot image.

Host checks: `make test-boot`.
