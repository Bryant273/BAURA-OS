#ifndef BAURA_DEVICE_MGR_H
#define BAURA_DEVICE_MGR_H

#include <stdint.h>
#include "../baura_core_types.h"

#define BAURA_DEVICE_CAPACITY 256u
#define BAURA_DRIVER_PROBE_CAPACITY 64u
#define BAURA_PCI_BAR_COUNT 6u

/*
 * Broad device categories derived from the PCI class code. Values 0-4 are
 * stable; new categories are appended.
 */
typedef enum {
    DEV_TYPE_STORAGE = 0,
    DEV_TYPE_NETWORK = 1,
    DEV_TYPE_GPU = 2,
    DEV_TYPE_INPUT = 3,     /* PCI class 0x09 input device controllers */
    DEV_TYPE_OTHER = 4,
    DEV_TYPE_USB_HOST = 5,  /* PCI class 0x0C, subclass 0x03 */
    DEV_TYPE_BRIDGE = 6,    /* PCI class 0x06 */
    DEV_TYPE_AUDIO = 7,     /* PCI class 0x04, subclass 0x01 or 0x03 */
    DEV_TYPE_WIRELESS = 8   /* PCI class 0x0D */
} DeviceType;

#define BAURA_DEVICE_TYPE_COUNT 9u

typedef struct {
    /* Stable for the lifetime of the registry: a device that is found again
     * by a later scan at the same location keeps its identifier. */
    uint32_t device_id;
    uint16_t vendor_id;
    uint16_t product_id;
    uint16_t subsystem_vendor_id;  /* header type 0 only, otherwise 0 */
    uint16_t subsystem_id;         /* header type 0 only, otherwise 0 */
    uint8_t class_code;
    uint8_t subclass;
    uint8_t programming_interface;
    uint8_t revision;
    uint8_t header_type;
    uint8_t bus;
    uint8_t slot;
    uint8_t function;
    uint8_t irq_line;
    /* Offsets of the capability structures in configuration space, or 0 when
     * the device does not expose them. */
    uint8_t msi_cap_offset;
    uint8_t msix_cap_offset;
    uint8_t pcie_cap_offset;
    uint8_t bar_count;
    uint8_t bar_valid_mask;
    DeviceType type;
    /* Raw PCI BAR dwords, not mapped addresses or pointers. */
    uint32_t bars[BAURA_PCI_BAR_COUNT];
    void *driver_handle;
    uint8_t driver_attached;
} BauraDevice;

typedef int (*BauraPciConfigRead)(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset, uint32_t *value, void *context);
typedef BauraStatus (*BauraDeviceProbe)(BauraDevice *device, void *context);

void dev_device_manager_init(void);
/* Installs a platform-supplied reader; Core does not access PCI ports here.
 * The reader returns non-zero on success and 0 when the read failed. */
BauraStatus dev_set_pci_reader(BauraPciConfigRead reader, void *context);
/* Scans the bus and rebuilds the device list. Devices found again at the same
 * location keep their identifier and driver binding. */
BauraStatus dev_discover_pci(void);
BauraStatus dev_register_driver(DeviceType type, BauraDeviceProbe probe, void *context);
BauraStatus dev_register_pci_driver(uint16_t vendor_id, uint16_t product_id,
                                    BauraDeviceProbe probe, void *context);
uint32_t dev_device_count(void);
BauraStatus dev_get_device(uint32_t index, BauraDevice *device);

#endif
