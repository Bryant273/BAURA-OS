# Device discovery and driver status

## Current implementation

BAURA Core contains a host-testable PCI inventory registry in
`DEVICE_MANAGER/BAURA_DEVICE_MGR.c`. It enumerates bus/slot/function data
through the injected `BauraPciConfigRead` callback, classifies broad PCI
classes, and invokes registered C probe callbacks. The Core startup initializes
the registry, but does not install a PCI reader. The default Multiboot image
and the x86_64 Core archive therefore do not detect physical devices.

The current interfaces are scaffolding, not a complete PnP or driver stack:

| Capability | State |
| --- | --- |
| PC serial console | A 16550-compatible COM1 driver is integrated in the existing 32-bit Multiboot kernel using direct ring-0 port I/O; intended for QEMU/compatible legacy PC boot, not seL4 |
| PCI inventory parsing | Implemented against an injected configuration-space reader; host fixture only |
| PCI hardware access | No real backend; the older `pci_scanner.c` is not in the build and is not a seL4 adapter |
| Device classification | Broad PCI class/subclass groups only |
| Driver matching | Exact PCI vendor/device ID registration plus legacy broad type matching |
| Driver execution | In-process C probe callback; no driver image loading, isolation, task lifecycle, or resource delegation |
| Driver lifecycle | Rust crate defines a platform-backend contract; no concrete seL4 backend is bound |
| Resource assignment | Not implemented; BAR values are retained as raw PCI config dwords with a validity mask and are not mapped or authorized |
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

For the x86_64 seL4 target, a platform adapter must obtain device configuration
and resource authority through the selected boot/capability setup, validate
that authority, and expose a narrow backend to Core. Core policy must not do
port I/O, map BARs, assign IRQs, or mint seL4 capabilities itself. The current
Makefile builds the seL4 kernel and Core archive separately; there is no such
adapter in the integrated boot path.

The other files under `KERNEL/L4` that contain custom x86_64 paging, interrupt,
or syscall fragments are not wired into the configured seL4 build and are not
the selected device backend. They must not be mixed into the seL4 kernel path.

## Hardware vendor and bus scope

PCI vendor IDs can identify a PCI function, not a complete retail product,
driver compatibility, or support level. Examples include Intel `8086`, NVIDIA
`10DE`, AMD `1002`, Qualcomm/Atheros `168C` and Qualcomm `17CB`, and Samsung
`144D`. These IDs are examples for cataloguing and matching only; no vendor
driver is included or implied.

Support must be built per device ID, revision, bus, firmware and required
features. PCI enumeration alone cannot find Logitech USB peripherals
(commonly USB VID `046D`), USB devices generally, I2C/SPI devices, or most
platform-described ARM devices. Snapdragon systems require an ARM64 platform
path and the firmware-provided ACPI/device-tree description, plus drivers for
the actual controllers and peripherals. Bluetooth devices normally require a
working USB/UART controller, transport driver, firmware and Bluetooth stack.

## Next implementation gates

1. Select and boot the real x86_64 seL4 initial task; document its BootInfo and
   device/I/O/IRQ capabilities.
2. Implement a seL4-specific PCI configuration/resource backend outside the
   portable Core. Validate it against the exact generated seL4 API and provide
   negative tests for missing or insufficient capabilities.
3. Replace in-process probe callbacks with versioned driver metadata, exact
   match tables, explicit resource requirements, and isolated task loading
   through the platform backend. Keep broad class matching as fallback only.
4. Add bus-specific discovery and services (ACPI/PCIe/USB for the selected PC
   target); add a separate ARM64 ACPI/device-tree path before claiming
   Snapdragon support.
5. Define device-management API/ABI operations only when their authorization,
   lifetime, hotplug, metadata and error contracts are complete.
6. Validate using emulated PCI devices, malformed configuration responses,
   unsupported IDs, resource conflicts and real hardware. Host fixture tests
   do not establish physical-device support.
