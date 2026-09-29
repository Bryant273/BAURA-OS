#ifndef BAURA_DEVICE_MGR_H
#define BAURA_DEVICE_MGR_H

#include <stdint.h>
#include "../baura_core_types.h"

#define BAURA_DEVICE_CAPACITY 128u
#define BAURA_DRIVER_PROBE_CAPACITY 16u

typedef enum {
    DEV_TYPE_STORAGE = 0,
    DEV_TYPE_NETWORK = 1,
    DEV_TYPE_GPU = 2,
    DEV_TYPE_INPUT = 3,
    DEV_TYPE_OTHER = 4
} DeviceType;

typedef struct {
    uint32_t device_id;
    uint16_t vendor_id;
    uint16_t product_id;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t bus;
    uint8_t slot;
    uint8_t function;
    uint8_t irq_line;
    DeviceType type;
    uintptr_t bar0;
    void *driver_handle;
    uint8_t driver_attached;
} BauraDevice;

typedef int (*BauraPciConfigRead)(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset, uint32_t *value, void *context);
typedef BauraStatus (*BauraDeviceProbe)(BauraDevice *device, void *context);

void dev_device_manager_init(void);
BauraStatus dev_set_pci_reader(BauraPciConfigRead reader, void *context);
BauraStatus dev_discover_pci(void);
BauraStatus dev_register_driver(DeviceType type, BauraDeviceProbe probe, void *context);
uint32_t dev_device_count(void);
BauraStatus dev_get_device(uint32_t index, BauraDevice *device);

#endif
