#ifndef BAURA_DRIVER_MANAGER_H
#define BAURA_DRIVER_MANAGER_H

#include "../DEVICE_MANAGER/BAURA_DEVICE_MGR.h"

void driver_manager_init(void);
BauraStatus baura_driver_register(DeviceType type, BauraDeviceProbe probe, void *context);
BauraStatus baura_driver_register_pci(uint16_t vendor_id, uint16_t product_id,
                                      BauraDeviceProbe probe, void *context);

#endif
