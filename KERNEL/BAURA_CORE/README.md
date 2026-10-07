# BAURA Core status

BAURA Core is a freestanding service layer above the low-level kernel. The legacy boot image is a 32-bit Multiboot bring-up path; it does not start seL4, user tasks, PCI/USB drivers, or a desktop. Its only physical-I/O driver in that path is the COM1 serial console. The default `make` target builds separate x86_64 Core, seL4 kernel, and BAURA initial-task ELF artifacts; the GRUB ISO and its boot are separate and not yet verified.

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
- Device and driver managers: PCI enumeration through an injected config-space reader, broad class classification, and in-process probe callbacks. Exact vendor/device registration is supported, but the boot image has no PCI backend, resource assignment, isolated driver loading, or device ABI. See [`DEVICE_DRIVER_STATUS.md`](./DEVICE_DRIVER_STATUS.md).
- Rust driver isolation crate: exposes a versioned platform-backend contract for spawn, liveness checks and termination; calls fail explicitly until a backend is installed. It no longer fabricates task/address-space identifiers. This contract is not itself a seL4 adapter; host tests exercise only the injected backend contract.
- Hardware manager: summarizes discovered devices when a platform PCI reader is installed.
- Storage and volume manager: bounded volume registry with LBA checks over injected block callbacks; no physical block driver is installed at boot.
- Network: frame transmit facade with MTU validation over an injected NIC callback; no network stack or NIC backend is installed.
- Power manager: profile state applied by an injected platform callback; no hardware policy backend is installed.
- Update manager: verify/apply/rollback state machine delegated to backend callbacks; it does not download or cryptographically verify packages itself.
- Virtualization: provider-gated VM lifecycle; no hypervisor provider is installed in the boot image.
- System API: guarded facade for configuration, process queries, and logs.
- Security: bounded C capability policy tables with revocation and attenuating delegation, initialized at boot. They are not seL4 capabilities. The initial-task platform can bind an explicit authority-check callback; authorization returns `BAURA_ERR_INVALID_STATE` when no checker is installed rather than allowing by default. The callback still needs a concrete seL4 service/capability implementation.
- VFS: the C path API resolves canonical `PC/<volume>:/...` paths against registered volume IDs and normalizes separators and `.` components while rejecting traversal. The Rust parser remains a separate reference implementation. There is still no mounted filesystem operation table or physical filesystem implementation.
- System API/ABI v2: `SYSTEM_API/API_ABI_v2.json` defines the normative wire contract and `SYSTEM_API/baura_abi_v2.h` exposes its scalar types, stable status/error values, capability names, opcodes, and reference-operation IDs. `SYSTEM_API/baura_ipc_v2.c` encodes/decodes frames byte-by-byte and validates the five reference request/response shapes. `make test-abi` first checks JSON/header consistency with the standard-library Python checker, then runs host codec tests; it is included by `make test-core`. ABI codec behavior is host-tested, but there is not yet a seL4 IPC transport or service dispatcher, nor filesystem/process/memory/network backends.
- Initial task entry: `SYSTEM_API/baura_core_initial_task_start` requires a versioned platform contract, an opaque boot-info pointer, and a platform-supplied validator before initializing Core. `BOOT/SEL4/initial_task.c` is the x86_64 seL4 adapter: it consumes BootInfo using the configured seL4 headers, validates the fixed boot-info fields and capability-slot regions, and uses `seL4_DebugPutChar` for console output. `BOOT/SEL4/initial_task_start.S` establishes a userspace stack and passes the initial `RDI` argument to the C entry. It deliberately installs no process or authority backend, so Core's protected operations continue to fail closed. The adapter has not yet been demonstrated by a successful build or boot, and it does not map framebuffer/input devices or launch isolated services.
- UART console adapter: `SYSTEM_API/baura_uart_console_write` adapts the BOOT UART driver to BAURA Core's console callback. The 16550-compatible COM1 serial driver is the first hardware driver integrated into the legacy boot path: `kernel_main` supplies explicit ring-0 x86 port-I/O callbacks, bounded transmit polling, and a 115200-baud divisor. This is for the existing 32-bit Multiboot/QEMU path only; it does not establish a seL4 adapter or broad PCI/USB device support.

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

`make` and `make baura-x86_64` build the x86_64 Core archive, seL4 kernel, and a separate BAURA initial-task ELF. The Core is linked into the user-task ELF; it is not linked into the seL4 kernel ELF. The seL4 build explicitly selects `KernelPlatform=pc99`, `KernelSel4Arch=x86_64`, and disables `KernelVerificationBuild` to permit kernel serial printing; this is a debug bring-up configuration, not a claim of a verified kernel. In WSL, the native x86_64 toolchain can be selected with `X86_64_CROSS_COMPILE=`; otherwise the default is `x86_64-elf-gcc`/`x86_64-elf-ar`. `make sel4-kernel-x86_64` builds seL4's `kernel.elf` and stages its configuration-generated libsel4 headers. `make sel4-initial-task-x86_64` builds `build/x86_64/baura_init.elf`. `make sel4-iso-x86_64` checks the kernel's Multiboot2 header and creates a GRUB ISO with seL4 as the kernel and the BAURA ELF as its initial-task module; `make run-sel4-x86_64` launches that ISO in QEMU. This is an emulator bring-up path, not proof of a UEFI boot. A cross CMake toolchain can be supplied with `SEL4_CMAKE_ARGS`, for example `SEL4_CMAKE_ARGS="-DCMAKE_TOOLCHAIN_FILE=/path/to/x86_64.cmake"`. `make test-core` builds/runs host-side C integration and ABI codec tests; `make test-core-x86_64` uses a 64-bit host ABI. `make test-rust` runs the Rust VFS, security, and driver-isolation contract unit tests. The old image is explicitly available as `make legacy32`, `make legacy32-iso`, and `make run-legacy32`; `make iso` and `make run` remain compatibility aliases for the legacy path only.

The seL4 target creates the generated header, metadata, and prune-header output
directories for the selected pc99/x86_64 configuration before invoking the
build, as some upstream generators open destination files directly and do not
create their parent directories. It also explicitly builds the upstream
`linker_ld_wrapper` target before `kernel.elf`, ensuring the preprocessed linker
script is available as a declared build target dependency.

The targeted host suites for the adjacent boot, communication, and GUI
contracts are `make test-boot`, `make test-communication`, and `make test-gui`;
`make test-integration` runs these plus `test-core`. Passing these host tests
does not establish UEFI, seL4, hardware-I/O, IPC, or display-device integration.
`make validate-all` combines the host integration tests, Rust unit tests, the
64-bit host Core suite, and the x86_64 Core, initial-task, and seL4 kernel
builds. Use `make -k validate-all` to continue independent checks after a
failure. Creating and running the ISO are separate explicit targets; building
the image does not prove that GRUB or QEMU can boot it, and the seL4 build is
not a kernel test suite.

The security/network/API alignment plan is tracked separately in
[`ROADMAP_SECURITY_NETWORK_ABI.md`](./ROADMAP_SECURITY_NETWORK_ABI.md). The Rust
driver-isolation backend interface is currently only a platform contract;
there is no seL4 implementation bound to it in this repository.

`baura_core_initial_task_start` requires an opaque boot-info object and a
platform validator, initializes BAURA Core only if validation succeeds, and
reports readiness through a caller-provided console callback. An optional
size-gated callback can start graphical service integration only after Core
initialization. The API does not parse or fabricate a kernel-specific
`BootInfo` contract. The current adapter validates the fixed BootInfo fields
and capability-slot ranges, but does not yet provide authority checks,
display/input resources, or task lifecycle operations. The GUI callback does
not discover or map devices.

The adapter is based on the selected seL4 source contract: x86_64 places the
initial thread's `capRegister` in `RDI`, and `boot_sys.c` loads the first
Multiboot module as a user ELF. The ISO uses GRUB's Multiboot2 `module2`
directive accordingly. The adapter validates the generated `seL4_BootInfo`
type and uses the debug character syscall enabled by `KernelPrinting`; it does
not yet consume or exercise the received capabilities. The ELF, GRUB module
handoff, and QEMU execution still require verification in the Linux build
environment. This is a bring-up path, not yet evidence of a working OS boot.

## Next integrations

1. Run the C and Rust test targets on a machine with the toolchains installed.
2. Add real lock/interrupt rules before calling the Core from multiple tasks.
3. Connect process metadata to actual L4 task lifecycle and security CSpaces.
4. Add a block-device interface and volume registry; then connect VFS resolution to mounted filesystems.
5. Implement PCI/device enumeration behind a platform abstraction, followed by individual drivers.
6. Only then enable network, power, update, virtualization, and graphical services.
