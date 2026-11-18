#ifndef BAURA_PCI_BACKEND_H
#define BAURA_PCI_BACKEND_H

/*
 * PCI configuration-space backend using the legacy x86 mechanism #1
 * (address port 0xCF8, data port 0xCFC).
 *
 * This file contains no seL4 types: the actual port access is supplied through
 * BauraPciPortIo, so the logic can be tested on a host. The seL4 binding that
 * obtains an I/O-port capability and provides those callbacks is in
 * pci_backend_sel4.c.
 *
 * Limits of this mechanism: it reaches only the first 256 bytes of each
 * function's configuration space (no PCIe extended capabilities) and it is
 * not atomic, because the address and data accesses are two separate port
 * operations. Callers must serialise access; the backend only rejects a
 * re-entrant call, it does not lock. PCIe ECAM (MMIO) is the planned
 * replacement for extended configuration space.
 */

#include <stdint.h>

#include "../../KERNEL/BAURA_CORE/DEVICE_MANAGER/BAURA_DEVICE_MGR.h"

#define BAURA_PCI_CONFIG_ADDRESS_PORT 0x0CF8u
#define BAURA_PCI_CONFIG_DATA_PORT 0x0CFCu
/* Smallest I/O-port range covering both ports (0xCF8-0xCFB and 0xCFC-0xCFF). */
#define BAURA_PCI_CONFIG_PORT_FIRST 0x0CF8u
#define BAURA_PCI_CONFIG_PORT_LAST 0x0CFFu

typedef struct {
    BauraStatus (*out32)(void *context, uint16_t port, uint32_t value);
    BauraStatus (*in32)(void *context, uint16_t port, uint32_t *value);
    void *context;
} BauraPciPortIo;

typedef struct {
    BauraPciPortIo io;
    uint8_t busy;
} BauraPciPortBackend;

/* Value written to the address port for a dword-aligned configuration offset. */
uint32_t baura_pci_config_address(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset);

BauraStatus baura_pci_port_backend_init(BauraPciPortBackend *backend, const BauraPciPortIo *io);

/*
 * Implements BauraPciConfigRead; `context` must be the BauraPciPortBackend.
 * Returns 1 on success and 0 on failure. Failure covers: null arguments, slot
 * >= 32, function >= 8, an offset that is not dword-aligned, a port error, or
 * a re-entrant call.
 */
int baura_pci_port_config_read(uint8_t bus, uint8_t slot, uint8_t function,
                               uint8_t offset, uint32_t *value, void *context);

/*
 * Dword configuration write, same validation and failure rules as the read.
 * Returns 1 on success, 0 on failure. Only the BAR-sizing code (pci_bar.c)
 * should use it; drivers never receive this function (see
 * DRIVER_DOCS/05_MODELE_DE_SECURITE.md). The data port 0xCFC is inside the
 * granted 0xCF8-0xCFF range, so no extra capability is needed.
 */
int baura_pci_port_config_write(uint8_t bus, uint8_t slot, uint8_t function,
                                uint8_t offset, uint32_t value, void *context);

/* Installs the backend as the Core's PCI reader. The backend must outlive use. */
BauraStatus baura_pci_port_backend_install(BauraPciPortBackend *backend);

#endif
