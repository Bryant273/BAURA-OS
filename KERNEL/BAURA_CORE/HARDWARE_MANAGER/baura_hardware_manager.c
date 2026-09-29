#include "baura_hardware_manager.h"

static uint8_t manager_ready;

void hardware_manager_init(void) { manager_ready = 1; }

BauraStatus baura_hardware_refresh(void) {
    if (!manager_ready) return BAURA_ERR_INVALID_STATE;
    return dev_discover_pci();
}

BauraStatus baura_hardware_get_summary(BauraHardwareSummary *summary) {
    uint32_t index;
    BauraDevice device;
    if (!manager_ready) return BAURA_ERR_INVALID_STATE;
    if (summary == 0) return BAURA_ERR_INVALID_ARGUMENT;
    summary->total = 0;
    for (index = 0; index < 5; ++index) summary->by_type[index] = 0;
    for (index = 0; index < dev_device_count(); ++index) {
        if (dev_get_device(index, &device) != BAURA_OK) return BAURA_ERR_INVALID_STATE;
        ++summary->total; ++summary->by_type[(uint32_t)device.type];
    }
    return BAURA_OK;
}
