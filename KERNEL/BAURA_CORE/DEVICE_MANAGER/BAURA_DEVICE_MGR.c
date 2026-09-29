#include "BAURA_DEVICE_MGR.h"

typedef struct { DeviceType type; BauraDeviceProbe probe; void *context; uint8_t active; } DriverProbe;
static BauraDevice devices[BAURA_DEVICE_CAPACITY];
static DriverProbe drivers[BAURA_DRIVER_PROBE_CAPACITY];
static BauraPciConfigRead pci_reader;
static void *pci_context;
static uint32_t device_count;

static int read_config(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset, uint32_t *value) {
    return pci_reader != 0 && pci_reader(bus, slot, function, offset, value, pci_context);
}

static DeviceType classify(uint8_t class_code, uint8_t subclass) {
    if (class_code == 0x01) return DEV_TYPE_STORAGE;
    if (class_code == 0x02) return DEV_TYPE_NETWORK;
    if (class_code == 0x03) return DEV_TYPE_GPU;
    if (class_code == 0x0C && (subclass == 0x03 || subclass == 0x05 || subclass == 0x80)) return DEV_TYPE_INPUT;
    return DEV_TYPE_OTHER;
}

static void attach_matching_drivers(BauraDevice *device) {
    uint32_t index;
    for (index = 0; index < BAURA_DRIVER_PROBE_CAPACITY; ++index)
        if (drivers[index].active && drivers[index].type == device->type && drivers[index].probe(device, drivers[index].context) == BAURA_OK) { device->driver_attached = 1; return; }
}

void dev_device_manager_init(void) {
    uint32_t index;
    device_count = 0; pci_reader = 0; pci_context = 0;
    for (index = 0; index < BAURA_DRIVER_PROBE_CAPACITY; ++index) drivers[index].active = 0;
}

BauraStatus dev_set_pci_reader(BauraPciConfigRead reader, void *context) {
    if (reader == 0) return BAURA_ERR_INVALID_ARGUMENT;
    pci_reader = reader; pci_context = context; return BAURA_OK;
}

BauraStatus dev_register_driver(DeviceType type, BauraDeviceProbe probe, void *context) {
    uint32_t index;
    if (probe == 0 || (uint32_t)type > (uint32_t)DEV_TYPE_OTHER) return BAURA_ERR_INVALID_ARGUMENT;
    for (index = 0; index < BAURA_DRIVER_PROBE_CAPACITY; ++index) if (drivers[index].active && drivers[index].type == type) {
        drivers[index].probe = probe; drivers[index].context = context;
        for (uint32_t device = 0; device < device_count; ++device) if (devices[device].type == type && devices[device].driver_attached == 0) attach_matching_drivers(&devices[device]);
        return BAURA_OK;
    }
    for (index = 0; index < BAURA_DRIVER_PROBE_CAPACITY; ++index) if (!drivers[index].active) {
        drivers[index].type = type; drivers[index].probe = probe; drivers[index].context = context; drivers[index].active = 1;
        for (uint32_t device = 0; device < device_count; ++device) if (devices[device].type == type) attach_matching_drivers(&devices[device]);
        return BAURA_OK;
    }
    return BAURA_ERR_CAPACITY;
}

BauraStatus dev_discover_pci(void) {
    uint16_t bus;
    if (pci_reader == 0) return BAURA_ERR_INVALID_STATE;
    device_count = 0;
    for (bus = 0; bus < 256; ++bus) {
        uint8_t slot;
        for (slot = 0; slot < 32; ++slot) {
            uint32_t header;
            uint8_t function_limit = 1;
            uint8_t function;
            if (!read_config((uint8_t)bus, slot, 0, 0x0C, &header)) continue;
            if (((header >> 16) & 0x80u) != 0) function_limit = 8;
            for (function = 0; function < function_limit; ++function) {
                uint32_t identity, class_register, bar0, interrupt;
                BauraDevice *device;
                if (!read_config((uint8_t)bus, slot, function, 0x00, &identity) || (identity & 0xFFFFu) == 0xFFFFu) continue;
                if (!read_config((uint8_t)bus, slot, function, 0x08, &class_register)) continue;
                if (device_count == BAURA_DEVICE_CAPACITY) return BAURA_ERR_CAPACITY;
                bar0 = 0; interrupt = 0xFF;
                (void)read_config((uint8_t)bus, slot, function, 0x10, &bar0);
                (void)read_config((uint8_t)bus, slot, function, 0x3C, &interrupt);
                device = &devices[device_count++];
                device->device_id = device_count;
                device->vendor_id = (uint16_t)identity; device->product_id = (uint16_t)(identity >> 16);
                device->class_code = (uint8_t)(class_register >> 24); device->subclass = (uint8_t)(class_register >> 16);
                device->bus = (uint8_t)bus; device->slot = slot; device->function = function;
                device->irq_line = (uint8_t)interrupt; device->type = classify(device->class_code, device->subclass);
                device->bar0 = (bar0 & 1u) == 0 ? (uintptr_t)(bar0 & ~0xFu) : 0;
                device->driver_handle = 0; device->driver_attached = 0; attach_matching_drivers(device);
            }
        }
    }
    return BAURA_OK;
}

uint32_t dev_device_count(void) { return device_count; }

BauraStatus dev_get_device(uint32_t index, BauraDevice *device) {
    if (device == 0) return BAURA_ERR_INVALID_ARGUMENT;
    if (index >= device_count) return BAURA_ERR_NOT_FOUND;
    *device = devices[index]; return BAURA_OK;
}
