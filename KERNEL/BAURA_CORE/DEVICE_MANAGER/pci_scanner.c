
#include "BAURA_DEVICE_MGR.h"
#include <stdint.h>

// Ports PCI standards
#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

uint32_t pci_read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((uint32_t)0x80000000 |
                       ((uint32_t)bus << 16) |
                       ((uint32_t)slot << 11) |
                       ((uint32_t)func << 8) |
                       ((uint32_t)offset & 0xFC));

    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

void dev_discover_all() {
    for (int bus = 0; bus < 256; bus++) {
        for (int slot = 0; slot < 32; slot++) {
            uint32_t vendor_id = pci_read_config(bus, slot, 0, 0) & 0xFFFF;
            if (vendor_id != 0xFFFF) {
                // Périphérique trouvé ! On l'enregistre dans le Device Manager
                printf("[Hardware] Found Device: Bus %d, Slot %d, Vendor ID 0x%X\n", bus, slot, vendor_id);
                // On associe ici le driver correspondant (USB, GPU, Network)
            }
        }
    }
}
