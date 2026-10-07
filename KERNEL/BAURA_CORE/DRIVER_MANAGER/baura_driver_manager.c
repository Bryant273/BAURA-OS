#include "baura_driver_manager.h"

static uint8_t manager_ready;

void driver_manager_init(void) { manager_ready = 1; }

BauraStatus baura_driver_register(DeviceType type, BauraDeviceProbe probe, void *context) {
    if (!manager_ready) return BAURA_ERR_INVALID_STATE;
    return dev_register_driver(type, probe, context);
}

BauraStatus baura_driver_register_pci(uint16_t vendor_id, uint16_t product_id,
                                      BauraDeviceProbe probe, void *context) {
    if (!manager_ready) return BAURA_ERR_INVALID_STATE;
    return dev_register_pci_driver(vendor_id, product_id, probe, context);
}
