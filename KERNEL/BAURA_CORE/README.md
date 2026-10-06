# BAURA Core status

BAURA Core is a freestanding service layer above the low-level kernel. The current boot target is a 32-bit Multiboot bring-up image; it does not yet start seL4, user tasks, hardware drivers, or a desktop.

## Implemented in this milestone

- System manager: ordered, idempotent initialization of the services listed below.
- Configuration: bounded in-memory key/value store. It is not persisted across reboot.
- Event bus: bounded synchronous topic subscriptions with unsubscribe tokens. It is single-core and not yet synchronized for concurrent callers.
- Logging: fixed-size circular log with chronological reads.
- Service manager: bounded registry with explicit start/stop callbacks and state queries. Registration now requires a real start callback; callbacks must not block. The manager does not create seL4 service tasks by itself.
- Process manager: retains metadata-only registration for bookkeeping and adds a task-backend contract for launch, state changes, and termination. Launch fails explicitly until a platform backend is bound; no seL4 task-creation backend is wired in this repository yet.
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
- Security: bounded C capability policy tables with revocation and attenuating delegation, initialized at boot. They are not seL4 capabilities. The initial-task platform can bind an explicit authority-check callback; authorization returns `BAURA_ERR_INVALID_STATE` when no checker is installed rather than allowing by default. The callback still needs a concrete seL4 service/capability implementation.
- VFS: the C path API resolves canonical `PC/<volume>:/...` paths against registered volume IDs and normalizes separators and `.` components while rejecting traversal. The Rust parser remains a separate reference implementation. There is still no mounted filesystem operation table or physical filesystem implementation.
- System API/ABI v2: `SYSTEM_API/API_ABI_v2.json` defines the normative wire contract and `SYSTEM_API/baura_abi_v2.h` exposes its scalar types, stable status/error values, capability names, opcodes, and reference-operation IDs. `SYSTEM_API/baura_ipc_v2.c` encodes/decodes frames byte-by-byte and validates the five reference request/response shapes; `make test-abi` runs host codec tests and is included by `make test-core`. A freestanding x86_64 Core archive and an x86_64 seL4 kernel build target now exist, but the default boot target remains 32-bit Multiboot; no callable seL4 IPC transport, service dispatcher, or filesystem/process/memory/network backend is integrated. See `SYSTEM_API/API_ABI_v2_STATUS.md` for binding and implementation limits.
- Initial task entry: `SYSTEM_API/baura_core_initial_task_start` requires a versioned platform contract, an opaque boot-info pointer, and a platform-supplied validator before initializing Core. It rejects unsupported contract versions or undersized structures before calling platform callbacks. Its optional, size-gated GUI-start extension invokes a platform callback after Core initialization; the GUI bridge can register/start its runtime through the Core service manager. This keeps the BAURA API independent of a guessed or kernel-specific `BootInfo` layout. The current code does not provide a seL4 validator, an initial-task executable, framebuffer mapping, or authorized PS/2 callbacks; a platform adapter must supply these before the graphical hook can start a real desktop.
- UART console adapter: `SYSTEM_API/baura_uart_console_write` adapts the BOOT UART driver to the Core console callback. Its port access remains injected and must be authorized by the running platform; it is not directly callable against hardware by default.

The seL4 source is currently declared as a Git submodule at `KERNEL/L4/sel4`
in `.gitmodules`. The separate roadmap
[`KERNEL/L4/ROADMAP_MICROKERNEL_BAURA.md`](../L4/ROADMAP_MICROKERNEL_BAURA.md)
describes the intended transition: use the checked-in seL4 version as an
unchanged reference/baseline, and eventually replace it with a BAURA-owned
microkernel implementation. Vendoring seL4 is a distinct repository operation:
it would copy and track the upstream source here, preserve its licensing
notices, and require deliberate update management; this checkout has not yet
been converted from a submodule.

## Build and tests

`make test-core` builds/runs the host-side C integration tests and ABI codec tests. `make test-core-x86_64` runs the C integration tests with a 64-bit host ABI and asserts the x86_64 target define. `make baura-core-x86_64` builds a freestanding x86_64 static Core archive using `x86_64-elf-gcc` and `x86_64-elf-ar`. `make sel4-kernel-x86_64` configures the checked-in seL4 source for its `pc99` x86_64 platform with kernel printing enabled, then builds the `kernel.elf` target. `make baura-x86_64` builds both artifacts. They remain separate: the Core archive is not yet a seL4 initial-task executable, and this target does not create a bootable combined image. A cross CMake toolchain can be supplied with `SEL4_CMAKE_ARGS`, for example `SEL4_CMAKE_ARGS="-DCMAKE_TOOLCHAIN_FILE=C:/toolchains/x86_64.cmake"`. `make test-rust` runs the Rust VFS and security unit tests. The legacy `make iso` and `make run` still build and launch the existing 32-bit Multiboot test image; they are not the x86_64/seL4 path.

The targeted host suites for the adjacent boot, communication, and GUI
contracts are `make test-boot`, `make test-communication`, and `make test-gui`;
`make test-integration` runs these plus `test-core`. Passing these host tests
does not establish UEFI, seL4, hardware-I/O, IPC, or display-device integration.

`baura_core_initial_task_start` requires an opaque boot-info object and a
platform validator, initializes BAURA Core only if validation succeeds, and
reports readiness through a caller-provided console callback. An optional
size-gated callback can start graphical service integration only after Core
initialization. The API does not parse or fabricate a kernel-specific
`BootInfo` contract. The platform adapter must validate the actual seL4 boot
information and capabilities and provide authorized console, display, and
input resources; the GUI callback does not discover or map devices.

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
