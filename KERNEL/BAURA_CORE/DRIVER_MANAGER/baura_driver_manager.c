#include "baura_driver_manager.h"

static uint8_t manager_ready;

void driver_manager_init(void) { manager_ready = 1; }

BauraStatus baura_driver_register(DeviceType type, BauraDeviceProbe probe, void *context) {
    if (!manager_ready) return BAURA_ERR_INVALID_STATE;
    return dev_register_driver(type, probe, context);
}
