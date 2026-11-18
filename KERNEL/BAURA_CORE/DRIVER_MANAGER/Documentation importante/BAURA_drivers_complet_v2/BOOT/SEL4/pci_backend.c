#include "pci_backend.h"

uint32_t baura_pci_config_address(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset) {
    return UINT32_C(0x80000000) |
           ((uint32_t)bus << 16) |
           ((uint32_t)(slot & 0x1Fu) << 11) |
           ((uint32_t)(function & 0x07u) << 8) |
           ((uint32_t)offset & 0xFCu);
}

BauraStatus baura_pci_port_backend_init(BauraPciPortBackend *backend, const BauraPciPortIo *io) {
    if (backend == 0 || io == 0 || io->in32 == 0 || io->out32 == 0)
        return BAURA_ERR_INVALID_ARGUMENT;
    backend->io = *io;
    backend->busy = 0;
    return BAURA_OK;
}

int baura_pci_port_config_read(uint8_t bus, uint8_t slot, uint8_t function,
                               uint8_t offset, uint32_t *value, void *context) {
    BauraPciPortBackend *backend = (BauraPciPortBackend *)context;
    uint32_t result = 0;
    int ok;
    if (backend == 0 || value == 0 || backend->io.in32 == 0 || backend->io.out32 == 0)
        return 0;
    if (slot >= 32u || function >= 8u || (offset & 0x3u) != 0) return 0;
    if (backend->busy) return 0;
    backend->busy = 1;
    ok = backend->io.out32(backend->io.context, BAURA_PCI_CONFIG_ADDRESS_PORT,
                           baura_pci_config_address(bus, slot, function, offset)) == BAURA_OK &&
         backend->io.in32(backend->io.context, BAURA_PCI_CONFIG_DATA_PORT, &result) == BAURA_OK;
    backend->busy = 0;
    if (!ok) return 0;
    *value = result;
    return 1;
}

int baura_pci_port_config_write(uint8_t bus, uint8_t slot, uint8_t function,
                                uint8_t offset, uint32_t value, void *context) {
    BauraPciPortBackend *backend = (BauraPciPortBackend *)context;
    int ok;
    if (backend == 0 || backend->io.in32 == 0 || backend->io.out32 == 0) return 0;
    if (slot >= 32u || function >= 8u || (offset & 0x3u) != 0) return 0;
    if (backend->busy) return 0;
    backend->busy = 1;
    ok = backend->io.out32(backend->io.context, BAURA_PCI_CONFIG_ADDRESS_PORT,
                           baura_pci_config_address(bus, slot, function, offset)) == BAURA_OK &&
         backend->io.out32(backend->io.context, BAURA_PCI_CONFIG_DATA_PORT, value) == BAURA_OK;
    backend->busy = 0;
    return ok;
}

BauraStatus baura_pci_port_backend_install(BauraPciPortBackend *backend) {
    if (backend == 0 || backend->io.in32 == 0 || backend->io.out32 == 0)
        return BAURA_ERR_INVALID_ARGUMENT;
    return dev_set_pci_reader(baura_pci_port_config_read, backend);
}
