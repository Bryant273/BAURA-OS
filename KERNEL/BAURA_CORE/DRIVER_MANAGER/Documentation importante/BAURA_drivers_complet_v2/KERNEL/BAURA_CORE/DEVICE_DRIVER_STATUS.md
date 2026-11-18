# Device discovery and driver status

The development plan for drivers (listing, matching, updates, driver sources)
is in [`DRIVER_DEVELOPMENT_GUIDE.md`](./DRIVER_DEVELOPMENT_GUIDE.md). This file
records what exists and how far it has been verified.

## Current implementation

BAURA Core contains a PCI inventory registry in
`DEVICE_MANAGER/BAURA_DEVICE_MGR.c`. It enumerates bus/slot/function data
through the injected `BauraPciConfigRead` callback, fills a `BauraDevice` per
function, classifies it, and invokes registered C probe callbacks.

For the x86_64 seL4 target the reader comes from two files in `BOOT/SEL4/`:

- `pci_backend.c`: portable logic for PCI configuration mechanism #1 (address
  port `0xCF8`, data port `0xCFC`). Port access is injected, so it has no seL4
  dependency.
- `pci_backend_sel4.c`: the seL4 binding. It issues one I/O-port capability for
  `0xCF8-0xCFF` from `seL4_CapIOPortControl` into the first free root-CNode
  slot (addressed with depth `seL4_WordBits`, because the root CNode has a guard)
  and installs the reader with `dev_set_pci_reader()`.

`BOOT/SEL4/initial_task.c` installs the backend after Core startup, calls
`baura_hardware_refresh()`, and prints one line per device on the debug
console (`DEVICE_MANAGER/baura_device_report.c`).

### Removed prototypes

`DEVICE_MANAGER/pci_scanner.c` and `pci_scanner_enhanced.c` were removed. They
were not part of the build, called `printf`, `outl` and `inl` without declaring
them, performed ring-0 port I/O that cannot run in an seL4 user task, and the
"enhanced" one only printed simulated ACPI messages. Their role is now covered
by `BOOT/SEL4/pci_backend*.c` and the registry. They remain available in git
history (last present in commit `86642c0`) if their content is needed.

### Verification state

- Covered by host tests (`make test-boot`, `make test-core`): configuration
  address encoding, argument validation, port-error handling, re-entrancy
  refusal, discovery against a fake PCI host (multi-function devices, a second
  bus, subsystem IDs, capability lists including a looping list), classification,
  stable identifiers across rescans, registry limits, and the console line format.
- **Not verified:** `pci_backend_sel4.c` has not been built against the
  generated seL4 headers, and nothing has booted. The call signatures were read
  from the pinned seL4 sources (`seL4_X86_IOPortControl_Issue`,
  `seL4_X86_IOPort_In32`, `seL4_X86_IOPort_Out32`). Whether the kernel grants
  the port range, and what the device list looks like on a booted system, is
  unknown until the initial task runs under QEMU or on hardware.

| Capability | State |
| --- | --- |
| PC serial console | A 16550-compatible COM1 driver is integrated in the existing 32-bit Multiboot kernel using direct ring-0 port I/O; intended for QEMU/compatible legacy PC boot, not seL4 |
| PCI inventory | Implemented; host-tested against a fake PCI host |
| PCI hardware access (seL4 x86_64) | Implemented in `BOOT/SEL4/`; unbuilt against generated seL4 headers and unbooted. Mechanism #1 reaches only the first 256 bytes of configuration space and is not atomic (callers must serialise) |
| Device identity | Vendor/device ID, subsystem vendor/ID (header type 0), class/subclass/programming interface, revision, bus/slot/function, IRQ line, MSI/MSI-X/PCIe capability offsets, and a `device_id` that stays stable across rescans |
| Classification | storage, network, GPU, input (PCI class `0x09`), USB host, bridge, audio, wireless, other |
| Driver matching | Exact PCI vendor/device ID first, then broad device type; the first successful probe wins. No subsystem, class/prog-if or priority matching yet |
| Registry capacity | 256 devices, 64 driver registrations |
| Rescan | A device found again at the same location keeps its `device_id` and driver binding; a device that disappeared leaves the list. There is no unbind callback for removed devices yet |
| Driver execution | In-process C probe callback; no driver image loading, isolation, task lifecycle, or resource delegation |
| Driver lifecycle | Rust crate defines a platform-backend contract; no concrete seL4 backend is bound |
| Storage driver | None. `USERSPACE/DRIVERS/STORAGE_DRIVERS` is a mock-up, not a driver: reads return dummy data, writes do nothing and report success, no register is accessed, and the crate does not compile (`uintptr_t` is not a Rust type; `cargo check` reports 3 errors) |
| Resource assignment | BAR sizing (`BOOT/SEL4/pci_bar.c`) and device-untyped lookup are implemented and host-tested against a fake device; nothing is mapped (no `Retype`/`Page_Map` code), assigned, or authorized, nothing has run on real hardware, and the registry still keeps raw BAR dwords. MSI/MSI-X are located but not configured |
| Non-PCI discovery | No USB, ACPI device enumeration, ACPI namespace, or device-tree discovery service |
| ABI exposure | No device-enumeration or driver-management operation in API/ABI v2 |

`driver_attached` means only that a registered probe callback returned
`BAURA_OK`. It does not mean a hardware driver was loaded or that the device is
usable.

## Core, seL4, and ABI boundary

The Core device registry is an internal policy/service interface, not a wire
ABI and not a kernel mechanism. If user applications need device enumeration
or management, define versioned BAURA API/ABI operations first, including
authorization, stable device identity, hotplug semantics, returned metadata,
and errors. Do not expose raw physical addresses, kernel capability slots, or
the current callback structures as an application ABI.

For the x86_64 seL4 target, the platform adapter obtains device configuration
and resource authority through the boot capability setup and exposes a narrow
backend to Core. Core policy must not do port I/O, map BARs, assign IRQs, or
mint seL4 capabilities itself; that stays in `BOOT/SEL4/`. The initial task
keeps one running free-slot cursor (`seL4_BootInfo.empty`) shared by everything
that needs new root-CNode slots.

The other files under `KERNEL/L4` that contain custom x86_64 paging, interrupt,
or syscall fragments are not wired into the configured seL4 build and are not
the selected device backend. They must not be mixed into the seL4 kernel path.

## Hardware vendor and bus scope

PCI vendor IDs can identify a PCI function, not a complete retail product,
driver compatibility, or support level. Examples include Intel `8086`, NVIDIA
`10DE`, AMD `1002` (GPU) and `1022` (chipset/CPU), Qualcomm/Atheros `168C` and
Qualcomm `17CB`, and Samsung `144D`. These IDs are examples for cataloguing and
matching only; no vendor driver is included or implied.

Support must be built per device ID, revision, bus, firmware and required
features. PCI enumeration alone cannot find Logitech USB peripherals
(commonly USB VID `046D`), USB devices generally, I2C/SPI devices, or most
platform-described ARM devices. Snapdragon systems require an ARM64 platform
path and the firmware-provided ACPI/device-tree description, plus drivers for
the actual controllers and peripherals. Bluetooth devices normally require a
working USB/UART controller, transport driver, firmware and Bluetooth stack.

## Next implementation gates

1. Select and boot the real x86_64 seL4 initial task; document its BootInfo and
   device/I/O/IRQ capabilities. Confirm on a booted system (QEMU `q35`) that the
   `0xCF8-0xCFF` capability is granted and that the device list printed by the
   initial task matches the machine's actual devices.
2. Add negative tests for the seL4 binding: missing or insufficient
   capabilities, an exhausted `empty` region.
   Before any run on a physical PC, follow the protocol in section 12 of
   `DRIVER_DEVELOPMENT_GUIDE.md` and `DRIVER_DOCS/01_DEMARRAGE_PC_REEL.md`: seL4's own documentation describes serial as
   the console on real hardware, and a community report shows a silent black
   screen on a bare-metal boot, so a framebuffer console is a prerequisite.
3. Replace in-process probe callbacks with versioned driver metadata, exact
   match tables, explicit resource requirements, and isolated task loading
   through the platform backend. Keep broad class matching as fallback only.
4. Add bus-specific discovery and services (ECAM/MCFG, ACPI, xHCI/USB for the
   selected PC target); add a separate ARM64 ACPI/device-tree path before
   claiming Snapdragon support.
5. Define device-management API/ABI operations only when their authorization,
   lifetime, hotplug, metadata and error contracts are complete.
6. Validate using emulated PCI devices, malformed configuration responses,
   unsupported IDs, resource conflicts and real hardware. Host fixture tests
   do not establish physical-device support.
