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
- System API/ABI v2: `SYSTEM_API/API_ABI_v2.json` defines the normative wire contract and `SYSTEM_API/baura_abi_v2.h` exposes its scalar types, stable status/error values, capability names, opcodes, and reference-operation IDs. `SYSTEM_API/baura_ipc_v2.c` encodes/decodes frames byte-by-byte and validates the five reference request/response shapes; `make test-abi` runs host codec tests and is included by `make test-core`. A freestanding x86_64 Core archive and an x86_64 seL4 kernel build target now exist, but the default boot target remains 32-bit Multiboot; no callable seL4 IPC transport, service dispatcher, or filesystem/process/memory/network backend is integrated. See `SYSTEM_API/API_ABI_v2_STATUS.md` for binding and implementation limits.
- Initial task entry: `SYSTEM_API/baura_core_initial_task_start` starts the existing BAURA Core services and reports readiness through an injected console callback. It deliberately consumes no guessed `BootInfo`; a real initial task must validate the seL4-provided boot information and capabilities and bind an authorized console service before invoking this entry point.

## Build and tests

`make test-core` builds/runs the host-side C integration tests and ABI codec tests. `make test-core-x86_64` runs the C integration tests with a 64-bit host ABI and asserts the x86_64 target define. `make baura-core-x86_64` builds a freestanding x86_64 static Core archive using `x86_64-elf-gcc` and `x86_64-elf-ar`. `make sel4-kernel-x86_64` configures the checked-in seL4 source for its `pc99` x86_64 platform with kernel printing enabled, then builds the `kernel.elf` target. `make baura-x86_64` builds both artifacts. They remain separate: the Core archive is not yet a seL4 initial-task executable, and this target does not create a bootable combined image. A cross CMake toolchain can be supplied with `SEL4_CMAKE_ARGS`, for example `SEL4_CMAKE_ARGS="-DCMAKE_TOOLCHAIN_FILE=C:/toolchains/x86_64.cmake"`. `make test-rust` runs the Rust VFS and security unit tests. The legacy `make iso` and `make run` still build and launch the existing 32-bit Multiboot test image; they are not the x86_64/seL4 path.

`baura_core_initial_task_start` initializes BAURA Core and reports readiness
through a caller-provided console callback. A future seL4 initial task must
validate the actual seL4 boot information and capabilities before calling it
and supply a console implementation through an authorized service; this API
does not parse or fabricate a local `BootInfo` contract.

The seL4 kernel target is an x86_64 kernel build only. It does not assert that
the resulting ELF is GRUB-loadable, does not create an initial task, and does
not combine the BAURA archive with seL4. Confirm the seL4 loader protocol,
initial-task image format, boot information, and capability handoff for this
exact configuration before wiring a UEFI/GRUB image.

## Next integrations

1. Run the C and Rust test targets on a machine with the toolchains installed.
2. Add real lock/interrupt rules before calling the Core from multiple tasks.
3. Connect process metadata to actual L4 task lifecycle and security CSpaces.
4. Add a block-device interface and volume registry; then connect VFS resolution to mounted filesystems.
5. Implement PCI/device enumeration behind a platform abstraction, followed by individual drivers.
6. Only then enable network, power, update, virtualization, and graphical services.
