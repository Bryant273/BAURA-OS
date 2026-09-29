#ifndef BAURA_DRIVER_MANAGER_H
#define BAURA_DRIVER_MANAGER_H

#include "../DEVICE_MANAGER/BAURA_DEVICE_MGR.h"

void driver_manager_init(void);
BauraStatus baura_driver_register(DeviceType type, BauraDeviceProbe probe, void *context);

#endif
