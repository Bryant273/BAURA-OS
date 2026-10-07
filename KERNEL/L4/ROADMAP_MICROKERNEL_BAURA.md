# BAURA L4 and microkernel roadmap

## Purpose and relationship to the existing roadmap

This document defines a separate development track for the L4 layer. It does
not replace or edit `BAURA OS — Roadmap du noyau et du système de boot.md`.
The existing roadmap describes the current seL4-based target; this one records
the longer-term intent to study that implementation and ultimately own the
microkernel that BAURA Core uses.

The long-term goal is a BAURA-owned x86_64 microkernel under `KERNEL/L4` that
provides the low-level mechanisms needed by BAURA Core. seL4 is the current
reference and build baseline, not a source tree to modify casually. No changes
to the seL4 implementation are required for the first BAURA initial-task
integration. A later replacement must be independently designed, implemented,
tested, and documented; it must not be represented as a small patch to seL4.

The checked-in seL4 source is presently a Git submodule at `KERNEL/L4/sel4`,
version `16.0.0-dev`. It has not been converted to ordinary tracked files or
renamed to `KERNEL/L4/SeL4`. That conversion is a deliberate repository
operation: remove the gitlink safely, preserve the complete source and license
notices, update build references, and verify a clean clone can build it. Do not
silently remove `.gitmodules` while leaving a submodule gitlink behind.

## Architecture boundaries

- The microkernel owns only mechanisms: address spaces, threads, scheduling,
  IPC endpoints, interrupt delivery, and kernel capability enforcement.
- BAURA Core owns policy and user-facing services: service supervision,
  authorization policy, VFS, storage/network services, configuration, and the
  BAURA system API.
- Kernel objects and capabilities must be passed through explicit, validated
  platform interfaces. BAURA software registries are not kernel capabilities.
- Kernel-specific boot structures must not leak into portable BAURA Core
  headers. The initial task adapter owns boot-protocol and capability checks.
- UEFI/GRUB remain external boot components. The exact loader protocol and
  initial-task ELF format must be verified for the selected kernel build.
- Existing `KERNEL/L4/MEMORY`, `SCHEDULER`, `api`, `arch`, and syscall fragments
  are not assumed to be integrated or authoritative until proven by build and
  execution.
- In particular, the unbuilt `KERNEL/L4/syscall_table.c` casts BAURA VFS and
  security functions to an incompatible generic syscall signature. It bypasses
  the BAURA IPC wire contract and must not be linked into seL4 or treated as a
  supported BAURA API/ABI. Replace it only as part of a separately specified
  BAURA-owned kernel syscall mechanism; BAURA service calls remain user-level
  IPC.

## Staged plan

### Stage 0 — establish a trustworthy reference baseline

1. Record the exact seL4 revision, version, target configuration, toolchain,
   license, and upstream state.
2. Keep seL4 unchanged while learning its boot, capability, IPC, and task
   contracts from its own headers, manual, and configured build.
3. Decide explicitly whether the repository will keep the submodule or vendor
   the unchanged source. For vendoring, preserve license/REUSE metadata and
   document how upstream updates will be tracked.

**Exit condition:** a reproducible x86_64 kernel build with the chosen source
management model, without claiming that this build starts BAURA.

### Stage 1 — define the BAURA platform boundary

1. Define a versioned, kernel-neutral initial-task platform contract. It must
   include only validated references/handles needed by Core, not a copied
   `seL4_BootInfo` structure.
2. Implement platform callbacks for boot-input validation, console output,
   task creation/lifecycle, and authority checks. Callbacks must return explicit
   status values; missing mandatory callbacks fail closed. The Core entry
   structure is ABI-versioned and size-checked.
3. Specify ownership, lifetime, error behavior, and which callbacks may block.
4. Keep the seL4-specific adapter outside the portable Core and compile it
   against the exact generated configuration/API for the selected build.

**Exit condition:** host tests exercise valid, invalid, and missing platform
inputs; no test fixture is described as a real seL4 boot.

### Stage 2 — build a real BAURA initial task on the seL4 baseline

1. Select the seL4 x86_64 platform and kernel options based on actual machine
   support; `pc99` is the currently configured candidate, not yet a validated
   hardware decision.
2. Establish the supported loader and kernel image format for this exact
   configuration, and document GRUB/UEFI requirements from verified sources.
3. Add the userspace build dependencies and startup/runtime needed to produce
   a BAURA initial-task executable; a static Core archive alone is insufficient.
4. The task validates actual BootInfo and received capabilities in its adapter,
   binds only capabilities it was handed, initializes Core, then reports
   readiness through a permitted console mechanism.
5. Produce a combined boot artifact only after the image contract and handoff
   are demonstrated. Validate first in the supported emulator, then on target
   hardware.

**Exit condition:** reproducible boot reaches a real BAURA userspace initial
task, logs readiness, and rejects invalid handoff data. A kernel-only build or
callback-only host test does not satisfy this condition.

### Stage 3 — connect BAURA services through the platform backend

1. Implement task lifecycle using actual kernel objects and capabilities.
2. Implement an IPC transport and ABI dispatcher with message/attachment
   semantics bound to the selected kernel API.
3. Add service task creation and capability distribution with least privilege.
4. Integrate storage only after a real block backend and mounted filesystem
   operations exist; resolve VFS paths through those mounted operations.
5. Add synchronization and concurrency rules before Core state is accessed
   from multiple tasks.

**Exit condition:** at least one isolated BAURA service is launched by the
initial task and completes a request/reply over the real kernel IPC path.

### Stage 4 — develop the BAURA-owned microkernel

1. Write a separate kernel specification: privilege model, boot contract,
   address-space model, thread state machine, scheduler policy, IPC semantics,
   capability representation, interrupt model, and security invariants.
2. Create an independently named kernel implementation directory under
   `KERNEL/L4` (proposed: `KERNEL/L4/BAURA_L4`) rather than editing the seL4
   reference source in place.
3. Bring up x86_64 in incremental, testable slices: entry/serial output,
   memory map and physical allocator, page tables/address spaces, exceptions
   and interrupts, threads/scheduling, IPC, then capabilities.
4. Add host/unit tests, emulator tests, negative security tests, and an explicit
   statement that no seL4 formal-verification guarantees transfer to BAURA L4.
5. Implement a BAURA initial task and the same abstract backend contract against
   BAURA L4. Migrate only after functional and security criteria are met.

**Exit condition:** BAURA L4 boots and provides the minimal mechanism contract
used by BAURA Core; seL4 remains an optional study/reference tree or is removed
only through a separately reviewed repository change.

## Current verified state

- The repository contains seL4 source reporting version `16.0.0-dev`.
- Its headers define `seL4_BootInfo`, including initial-thread IPC-buffer,
  image-frame and untyped-capability regions. This is evidence of the source
  API, not evidence that the current BAURA target receives or validates it.
- The Makefile configures `KernelPlatform=pc99`, `KernelSel4Arch=x86_64`,
  `KernelVerificationBuild=OFF`, and `KernelPrinting=ON`. The default target
  now requests the separate x86_64 Core archive, seL4 kernel, and BAURA
  initial-task ELF. `BOOT/SEL4` contains a seL4-specific adapter, startup
  assembly, user-image linker script, and GRUB Multiboot2 configuration. The
  seL4 bring-up config disables `KernelVerificationBuild` because this source
  gates `KernelPrinting` on verification being off; it is not a verified-
  kernel configuration.
- In the WSL Linux filesystem, `make sel4-kernel-x86_64` completed and produced
  `build/sel4-x86_64/kernel.elf`. `file` and `readelf` confirmed an ELF64
  x86-64 executable with entry point `0x10041e` and three `PT_LOAD` segments.
  GNU ld reported two load segments with RWX permissions; this warning remains
  to be investigated before treating the artifact as suitable beyond debug
  bring-up. This verifies compilation only, not boot.
- The selected upstream x86 boot implementation declares Multiboot 1 and
  Multiboot 2 headers enabled by default and `boot_sys.c` rejects boot if no
  module containing a userspace ELF image is supplied. The new ISO target
  configures seL4 as a Multiboot2 kernel and supplies `baura_init.elf` as the
  first module, matching that source-level contract. These new build and GRUB
  targets have not yet been run in the Linux build environment, and QEMU has
  not verified the handoff.
- BAURA Core's PCI registry accepts an injected config-space reader and has no
  physical-device backend. Its C driver probes are in-process callbacks; the
  Rust isolation interface has no concrete seL4 binding. The custom x86_64
  fragments under `KERNEL/L4` are not connected to the configured seL4 build.
  Device/resource authority must therefore be added in the seL4 platform
  adapter and passed through a checked Core interface; the portable Core must
  not access PCI ports or map resource addresses itself.
- `BOOT/BOOTLOADER/KERNEL_LOADER/L4_LOADER.c` and
  `BOOT/BOOTLOADER/KERNEL_HANDOFF.h` now define a versioned generic handoff
  and callback-based validation/transfer contract. This is not a kernel image
  loader or a seL4 boot protocol implementation.
- BAURA Core currently has injectable process and authority callbacks. The
  initial-task entry requires a supported platform ABI version, a sufficiently
  large platform structure, an opaque boot-info object, and a validation
  callback before system initialization. `BOOT/SEL4/initial_task.c` implements
  a first adapter using the generated seL4 BootInfo structure and debug
  console syscall. It installs neither process-creation nor authorization
  backends; capabilities are validated structurally but are not yet consumed.
- The legacy 32-bit Multiboot path now installs a ring-0 COM1 UART port-I/O
  backend for boot diagnostics. This is not a seL4 userspace adapter and grants
  no UART port capability to a BAURA task.
- BOOT validates callback-normalized firmware memory-map snapshots, but no
  native UEFI protocol adapter or boot-services handoff exists yet.
- COMMUNICATION now has a synchronous bounded C callback bus and a transport-
  injected RPC contract. Neither is a seL4 IPC transport.
- GUI display/surface/composition logic is host-testable against an injected
  framebuffer. No framebuffer mapping, GPU, input backend, or desktop startup
  is integrated.
- Host tests for boot, communication, GUI, ABI, Core and Rust were run
  successfully in WSL. The Core x86_64 archive also compiled with native WSL
  GCC in freestanding mode. The seL4 kernel ELF compiled successfully as
  recorded above. The initial-task adapter, combined ISO, and QEMU boot have
  not yet been built or verified; Stage 2 remains incomplete.

## Immediate next work

1. In WSL, build `make baura-x86_64 X86_64_CROSS_COMPILE=` and inspect the
   initial-task ELF headers and segments; fix any compiler/linker integration
   failures before proceeding.
2. Build `make sel4-iso-x86_64 X86_64_CROSS_COMPILE=` and confirm the GRUB
   Multiboot2 image/module handoff, then run `make run-sel4-x86_64
   X86_64_CROSS_COMPILE=` and verify BootInfo validation plus the Core-ready
   message in QEMU.
3. Add negative tests for malformed boot-info inputs and a real negative
   boot/handoff test where feasible; validate capability use before adding
   task creation or IPC services.
4. Choose and document the seL4 source-management model before changing
   `.gitmodules` or moving the source directory.
5. Keep the separate BAURA L4 kernel specification and implementation as a
   later workstream; do not mix speculative kernel code into the seL4 tree.
