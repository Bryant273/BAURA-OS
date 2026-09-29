# BAURA Core status

BAURA Core is a freestanding service layer above the low-level kernel. The current boot target is a 32-bit Multiboot bring-up image; it does not yet start seL4, user tasks, hardware drivers, or a desktop.

## Implemented in this milestone

- System manager: ordered, idempotent initialization of the services listed below.
- Configuration: bounded in-memory key/value store. It is not persisted across reboot.
- Event bus: bounded synchronous topic subscriptions with unsubscribe tokens. It is single-core and not yet synchronized for concurrent callers.
- Logging: fixed-size circular log with chronological reads.
- Service manager: bounded registry with explicit start/stop callbacks and state queries. Callbacks must not block.
- Process manager: process metadata and state transitions only; it does not create or schedule kernel tasks.
- Resource manager: per-owner counter quotas, initially deny-by-default. It is accounting, not a physical allocator.
- Monitoring and performance: fixed-size metric history and overflow-safe counters; sampling sources are not connected to hardware yet.
- Fault and recovery: bounded fault history and code-to-handler dispatch. Recovery handlers must implement any actual restart or remediation.
- Device and driver managers: callback-based PCI configuration enumeration/classification and driver probing. The boot image has no PCI configuration backend yet.
- Hardware manager: summarizes discovered devices when a platform PCI reader is installed.
- Storage and volume manager: bounded volume registry with LBA checks over injected block callbacks; no physical block driver is installed at boot.
- Network: frame transmit facade with MTU validation over an injected NIC callback; no network stack or NIC backend is installed.
- Power manager: profile state applied by an injected platform callback; no hardware policy backend is installed.
- Update manager: verify/apply/rollback state machine delegated to backend callbacks; it does not download or cryptographically verify packages itself.
- Virtualization: provider-gated VM lifecycle; no hypervisor provider is installed in the boot image.
- System API: guarded facade for configuration, process queries, and logs.
- Security: bounded C capability spaces with revocation and attenuating delegation, initialized at boot. The tables are single-core and kernel-internal; caller authorization and concurrency protection are not yet connected to seL4 capabilities. The Rust crate remains a separate reference/test implementation.
- VFS parser: bounded canonical path parsing in Rust with unit tests. Volume discovery and physical filesystem operations are not connected yet.

## Build and tests

`make test-core` builds/runs the host-side C integration tests. `make test-rust` runs the Rust VFS and security unit tests. `make iso` creates the Multiboot test image and `make run` launches it in QEMU when the required cross-compiler, NASM, GRUB tools, and QEMU are installed.

## Next integrations

1. Run the C and Rust test targets on a machine with the toolchains installed.
2. Add real lock/interrupt rules before calling the Core from multiple tasks.
3. Connect process metadata to actual L4 task lifecycle and security CSpaces.
4. Add a block-device interface and volume registry; then connect VFS resolution to mounted filesystems.
5. Implement PCI/device enumeration behind a platform abstraction, followed by individual drivers.
6. Only then enable network, power, update, virtualization, and graphical services.
