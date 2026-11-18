#include "BAURA_DEVICE_MGR.h"

#define PCI_CAP_ID_PCIE 0x10u
#define PCI_CAP_ID_MSI 0x05u
#define PCI_CAP_ID_MSIX 0x11u
/* A valid list lives in offsets 0x40..0xFC, four bytes per entry at minimum. */
#define PCI_CAP_LIST_MAX_ENTRIES 48u
#define PCI_CAP_LIST_FIRST_OFFSET 0x40u

typedef struct {
    DeviceType type;
    uint16_t vendor_id;
    uint16_t product_id;
    BauraDeviceProbe probe;
    void *context;
    uint8_t match_pci_id;
    uint8_t active;
} DriverProbe;

/* What a rescan needs to remember about a device found by the previous scan. */
typedef struct {
    uint32_t device_id;
    uint16_t vendor_id;
    uint16_t product_id;
    uint8_t bus;
    uint8_t slot;
    uint8_t function;
    uint8_t driver_attached;
    void *driver_handle;
} KnownDevice;

static BauraDevice devices[BAURA_DEVICE_CAPACITY];
static KnownDevice known[BAURA_DEVICE_CAPACITY];
static DriverProbe drivers[BAURA_DRIVER_PROBE_CAPACITY];
static BauraPciConfigRead pci_reader;
static void *pci_context;
static uint32_t device_count;
static uint32_t known_count;
static uint32_t next_device_id;

static int read_config(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset, uint32_t *value) {
    return pci_reader != 0 && pci_reader(bus, slot, function, offset, value, pci_context);
}

static DeviceType classify(uint8_t class_code, uint8_t subclass) {
    switch (class_code) {
    case 0x01: return DEV_TYPE_STORAGE;
    case 0x02: return DEV_TYPE_NETWORK;
    case 0x03: return DEV_TYPE_GPU;
    case 0x04: return (subclass == 0x01 || subclass == 0x03) ? DEV_TYPE_AUDIO : DEV_TYPE_OTHER;
    case 0x06: return DEV_TYPE_BRIDGE;
    case 0x09: return DEV_TYPE_INPUT;
    case 0x0C: return subclass == 0x03 ? DEV_TYPE_USB_HOST : DEV_TYPE_OTHER;
    case 0x0D: return DEV_TYPE_WIRELESS;
    default: return DEV_TYPE_OTHER;
    }
}

/*
 * Walks the capability list. The walk is bounded so that a malformed or
 * looping list cannot hang discovery; a failed read ends the walk and keeps
 * what was found so far.
 */
static void read_capabilities(BauraDevice *device) {
    uint32_t status_command, pointer_register, pointer, hops;
    if (device->header_type > 1u) return;
    if (!read_config(device->bus, device->slot, device->function, 0x04, &status_command)) return;
    if (((status_command >> 16) & 0x10u) == 0) return; /* status bit 4: capability list present */
    if (!read_config(device->bus, device->slot, device->function, 0x34, &pointer_register)) return;
    pointer = pointer_register & 0xFCu;
    for (hops = 0; hops < PCI_CAP_LIST_MAX_ENTRIES && pointer >= PCI_CAP_LIST_FIRST_OFFSET; ++hops) {
        uint32_t header;
        uint8_t id;
        if (!read_config(device->bus, device->slot, device->function, (uint8_t)pointer, &header)) return;
        id = (uint8_t)header;
        if (id == PCI_CAP_ID_MSI && device->msi_cap_offset == 0) device->msi_cap_offset = (uint8_t)pointer;
        else if (id == PCI_CAP_ID_MSIX && device->msix_cap_offset == 0) device->msix_cap_offset = (uint8_t)pointer;
        else if (id == PCI_CAP_ID_PCIE && device->pcie_cap_offset == 0) device->pcie_cap_offset = (uint8_t)pointer;
        pointer = (header >> 8) & 0xFCu;
    }
}

static void attach_matching_drivers(BauraDevice *device) {
    uint32_t index;
    uint8_t exact_match_pass;
    for (exact_match_pass = 1; exact_match_pass <= 2; ++exact_match_pass) {
        for (index = 0; index < BAURA_DRIVER_PROBE_CAPACITY; ++index) {
            BauraDevice candidate;
            uint8_t matches;
            if (!drivers[index].active ||
                drivers[index].match_pci_id != (uint8_t)(exact_match_pass == 1))
                continue;
            matches = drivers[index].match_pci_id
                ? drivers[index].vendor_id == device->vendor_id &&
                  drivers[index].product_id == device->product_id
                : drivers[index].type == device->type;
            if (!matches) continue;
            candidate = *device;
            if (drivers[index].probe(&candidate, drivers[index].context) == BAURA_OK) {
                device->driver_handle = candidate.driver_handle;
                device->driver_attached = 1;
                return;
            }
        }
    }
}

static const KnownDevice *find_known(const BauraDevice *device) {
    uint32_t index;
    for (index = 0; index < known_count; ++index) {
        const KnownDevice *entry = &known[index];
        if (entry->bus == device->bus && entry->slot == device->slot &&
            entry->function == device->function &&
            entry->vendor_id == device->vendor_id &&
            entry->product_id == device->product_id)
            return entry;
    }
    return 0;
}

static void remember_current_devices(void) {
    uint32_t index;
    for (index = 0; index < device_count; ++index) {
        known[index].device_id = devices[index].device_id;
        known[index].vendor_id = devices[index].vendor_id;
        known[index].product_id = devices[index].product_id;
        known[index].bus = devices[index].bus;
        known[index].slot = devices[index].slot;
        known[index].function = devices[index].function;
        known[index].driver_attached = devices[index].driver_attached;
        known[index].driver_handle = devices[index].driver_handle;
    }
    known_count = device_count;
}

void dev_device_manager_init(void) {
    uint32_t index;
    device_count = 0; known_count = 0; next_device_id = 1;
    pci_reader = 0; pci_context = 0;
    for (index = 0; index < BAURA_DRIVER_PROBE_CAPACITY; ++index) drivers[index].active = 0;
}

BauraStatus dev_set_pci_reader(BauraPciConfigRead reader, void *context) {
    if (reader == 0) return BAURA_ERR_INVALID_ARGUMENT;
    pci_reader = reader; pci_context = context; return BAURA_OK;
}

BauraStatus dev_register_driver(DeviceType type, BauraDeviceProbe probe, void *context) {
    uint32_t index;
    if (probe == 0 || (uint32_t)type >= BAURA_DEVICE_TYPE_COUNT) return BAURA_ERR_INVALID_ARGUMENT;
    for (index = 0; index < BAURA_DRIVER_PROBE_CAPACITY; ++index) if (drivers[index].active && !drivers[index].match_pci_id && drivers[index].type == type) {
        drivers[index].probe = probe; drivers[index].context = context;
        for (uint32_t device = 0; device < device_count; ++device) if (devices[device].type == type && devices[device].driver_attached == 0) attach_matching_drivers(&devices[device]);
        return BAURA_OK;
    }
    for (index = 0; index < BAURA_DRIVER_PROBE_CAPACITY; ++index) if (!drivers[index].active) {
        drivers[index].type = type; drivers[index].probe = probe; drivers[index].context = context;
        drivers[index].match_pci_id = 0; drivers[index].active = 1;
        for (uint32_t device = 0; device < device_count; ++device)
            if (devices[device].type == type &&
                devices[device].driver_attached == 0)
                attach_matching_drivers(&devices[device]);
        return BAURA_OK;
    }
    return BAURA_ERR_CAPACITY;
}

BauraStatus dev_register_pci_driver(uint16_t vendor_id, uint16_t product_id,
                                    BauraDeviceProbe probe, void *context) {
    uint32_t index;
    if (vendor_id == 0xFFFFu || probe == 0) return BAURA_ERR_INVALID_ARGUMENT;
    for (index = 0; index < BAURA_DRIVER_PROBE_CAPACITY; ++index) {
        if (drivers[index].active && drivers[index].match_pci_id &&
            drivers[index].vendor_id == vendor_id &&
            drivers[index].product_id == product_id) {
            drivers[index].probe = probe;
            drivers[index].context = context;
            for (uint32_t device = 0; device < device_count; ++device)
                if (devices[device].vendor_id == vendor_id &&
                    devices[device].product_id == product_id &&
                    devices[device].driver_attached == 0)
                    attach_matching_drivers(&devices[device]);
            return BAURA_OK;
        }
    }
    for (index = 0; index < BAURA_DRIVER_PROBE_CAPACITY; ++index) {
        if (!drivers[index].active) {
            drivers[index].vendor_id = vendor_id;
            drivers[index].product_id = product_id;
            drivers[index].probe = probe;
            drivers[index].context = context;
            drivers[index].match_pci_id = 1;
            drivers[index].active = 1;
            break;
        }
    }
    if (index == BAURA_DRIVER_PROBE_CAPACITY) return BAURA_ERR_CAPACITY;
    for (index = 0; index < device_count; ++index)
        if (devices[index].vendor_id == vendor_id &&
            devices[index].product_id == product_id &&
            devices[index].driver_attached == 0)
            attach_matching_drivers(&devices[index]);
    return BAURA_OK;
}

BauraStatus dev_discover_pci(void) {
    uint16_t bus;
    if (pci_reader == 0) return BAURA_ERR_INVALID_STATE;
    remember_current_devices();
    device_count = 0;
    for (bus = 0; bus < 256; ++bus) {
        uint8_t slot;
        for (slot = 0; slot < 32; ++slot) {
            uint32_t header, function_zero_identity;
            uint8_t function_limit = 1;
            uint8_t function;
            if (!read_config((uint8_t)bus, slot, 0, 0x00,
                             &function_zero_identity) ||
                (function_zero_identity & 0xFFFFu) == 0xFFFFu)
                continue;
            if (!read_config((uint8_t)bus, slot, 0, 0x0C, &header)) continue;
            if (((header >> 16) & 0x80u) != 0) function_limit = 8;
            for (function = 0; function < function_limit; ++function) {
                uint32_t identity, class_register, function_header, interrupt, subsystem;
                const KnownDevice *previous;
                BauraDevice *device;
                if (function == 0) identity = function_zero_identity;
                else if (!read_config((uint8_t)bus, slot, function, 0x00,
                                      &identity) ||
                         (identity & 0xFFFFu) == 0xFFFFu)
                    continue;
                if (!read_config((uint8_t)bus, slot, function, 0x08, &class_register) ||
                    !read_config((uint8_t)bus, slot, function, 0x0C, &function_header)) continue;
                if (device_count == BAURA_DEVICE_CAPACITY) return BAURA_ERR_CAPACITY;
                interrupt = 0xFF;
                (void)read_config((uint8_t)bus, slot, function, 0x3C, &interrupt);
                device = &devices[device_count++];
                *device = (BauraDevice){0};
                device->vendor_id = (uint16_t)identity; device->product_id = (uint16_t)(identity >> 16);
                device->revision = (uint8_t)class_register;
                device->programming_interface = (uint8_t)(class_register >> 8);
                device->subclass = (uint8_t)(class_register >> 16);
                device->class_code = (uint8_t)(class_register >> 24);
                device->header_type = (uint8_t)(function_header >> 16) & 0x7Fu;
                device->bus = (uint8_t)bus; device->slot = slot; device->function = function;
                device->irq_line = (uint8_t)interrupt; device->type = classify(device->class_code, device->subclass);
                device->bar_count = device->header_type == 0 ? 6u :
                                    (device->header_type == 1 ? 2u : 0u);
                for (uint8_t bar = 0; bar < device->bar_count; ++bar) {
                    if (read_config((uint8_t)bus, slot, function,
                                    (uint8_t)(0x10u + (bar * 4u)),
                                    &device->bars[bar]))
                        device->bar_valid_mask |= (uint8_t)(1u << bar);
                }
                if (device->header_type == 0 &&
                    read_config((uint8_t)bus, slot, function, 0x2C, &subsystem)) {
                    device->subsystem_vendor_id = (uint16_t)subsystem;
                    device->subsystem_id = (uint16_t)(subsystem >> 16);
                }
                read_capabilities(device);
                device->driver_handle = 0; device->driver_attached = 0;
                previous = find_known(device);
                if (previous != 0) {
                    device->device_id = previous->device_id;
                    if (previous->driver_attached) {
                        /* Same device as in the previous scan: keep its binding. */
                        device->driver_handle = previous->driver_handle;
                        device->driver_attached = 1;
                    }
                } else {
                    device->device_id = next_device_id++;
                }
                if (!device->driver_attached) attach_matching_drivers(device);
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
