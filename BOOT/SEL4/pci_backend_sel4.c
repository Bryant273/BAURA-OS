#include <stddef.h>
#include <stdint.h>
#include "pci_backend_sel4.h"
#include "pci_backend.h"

typedef struct {
    seL4_CPtr ioport;
} BauraSel4PciPorts;

static BauraSel4PciPorts pci_ports;
static BauraPciPortBackend pci_backend;
static uint8_t pci_installed;

static BauraStatus map_sel4_error(seL4_Error error) {
    switch (error) {
    case seL4_NoError:
        return BAURA_OK;
    case seL4_IllegalOperation:
    case seL4_InvalidCapability:
        return BAURA_ERR_PERMISSION;
    case seL4_DeleteFirst:
    case seL4_RevokeFirst:
        return BAURA_ERR_ALREADY_EXISTS;
    case seL4_InvalidArgument:
    case seL4_RangeError:
    case seL4_FailedLookup:
        return BAURA_ERR_INVALID_ARGUMENT;
    default:
        return BAURA_ERR_IO;
    }
}

static BauraStatus sel4_port_out32(void *context, uint16_t port, uint32_t value) {
    const BauraSel4PciPorts *ports = (const BauraSel4PciPorts *)context;
    return map_sel4_error(seL4_X86_IOPort_Out32(ports->ioport, port, value));
}

static BauraStatus sel4_port_in32(void *context, uint16_t port, uint32_t *value) {
    const BauraSel4PciPorts *ports = (const BauraSel4PciPorts *)context;
    seL4_X86_IOPort_In32_t result = seL4_X86_IOPort_In32(ports->ioport, port);
    BauraStatus status = map_sel4_error(result.error);
    if (status != BAURA_OK) return status;
    *value = result.result;
    return BAURA_OK;
}

BauraStatus baura_sel4_pci_install(const seL4_BootInfo *boot_info,
                                   seL4_SlotPos *next_free_slot) {
    BauraPciPortIo io;
    BauraStatus status;
    seL4_SlotPos slot;
    if (boot_info == NULL || next_free_slot == NULL) return BAURA_ERR_INVALID_ARGUMENT;
    if (pci_installed) return BAURA_ERR_ALREADY_EXISTS;
    slot = *next_free_slot;
    if (slot < boot_info->empty.start || slot >= boot_info->empty.end) return BAURA_ERR_CAPACITY;

    /*
     * The initial thread's root CNode has a guard of (word bits - size bits),
     * so a slot is addressed with a depth of seL4_WordBits, not with the
     * CNode's radix size.
     */
    status = map_sel4_error(seL4_X86_IOPortControl_Issue(
        seL4_CapIOPortControl, BAURA_PCI_CONFIG_PORT_FIRST, BAURA_PCI_CONFIG_PORT_LAST,
        seL4_CapInitThreadCNode, slot, seL4_WordBits));
    if (status != BAURA_OK) return status;
    *next_free_slot = slot + 1;

    pci_ports.ioport = slot;
    io.out32 = sel4_port_out32;
    io.in32 = sel4_port_in32;
    io.context = &pci_ports;
    status = baura_pci_port_backend_init(&pci_backend, &io);
    if (status != BAURA_OK) return status;
    status = baura_pci_port_backend_install(&pci_backend);
    if (status != BAURA_OK) return status;
    pci_installed = 1;
    return BAURA_OK;
}
