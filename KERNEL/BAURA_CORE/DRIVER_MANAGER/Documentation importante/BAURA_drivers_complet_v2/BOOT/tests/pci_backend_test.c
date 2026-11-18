/*
 * Host tests for the PCI port backend and the Core device registry.
 *
 * A fake PCI host models configuration mechanism #1: a write to port 0xCF8
 * selects a function and register, a read from 0xCFC returns that register.
 * Absent functions read as all ones, as on real hardware.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../SEL4/pci_backend.h"
#include "../../KERNEL/BAURA_CORE/DEVICE_MANAGER/baura_device_report.h"

#define FAKE_MAX_FUNCTIONS 24

typedef struct {
    uint8_t bus, slot, function;
    uint8_t present;
    uint32_t cfg[64];
} FakeFunction;

typedef struct {
    FakeFunction functions[FAKE_MAX_FUNCTIONS];
    uint32_t count;
    uint32_t address;
    int fail_out;
    int fail_in;
    unsigned out_calls;
    unsigned in_calls;
    BauraPciPortBackend *reenter;
    int reenter_result;
} FakeHost;

static BauraStatus fake_out32(void *context, uint16_t port, uint32_t value) {
    FakeHost *host = (FakeHost *)context;
    ++host->out_calls;
    assert(port == BAURA_PCI_CONFIG_ADDRESS_PORT);
    if (host->fail_out) return BAURA_ERR_IO;
    host->address = value;
    return BAURA_OK;
}

static BauraStatus fake_in32(void *context, uint16_t port, uint32_t *value) {
    FakeHost *host = (FakeHost *)context;
    uint32_t index;
    ++host->in_calls;
    assert(port == BAURA_PCI_CONFIG_DATA_PORT);
    if (host->fail_in) return BAURA_ERR_IO;
    if (host->reenter != 0) {
        uint32_t ignored;
        host->reenter_result = baura_pci_port_config_read(0, 0, 0, 0, &ignored, host->reenter);
    }
    *value = 0xFFFFFFFFu;
    if ((host->address & 0x80000000u) == 0) return BAURA_OK;
    for (index = 0; index < host->count; ++index) {
        const FakeFunction *fn = &host->functions[index];
        if (fn->present && fn->bus == ((host->address >> 16) & 0xFFu) &&
            fn->slot == ((host->address >> 11) & 0x1Fu) &&
            fn->function == ((host->address >> 8) & 0x07u)) {
            *value = fn->cfg[(host->address & 0xFCu) / 4u];
            return BAURA_OK;
        }
    }
    return BAURA_OK;
}

static FakeFunction *add_function(FakeHost *host, uint8_t bus, uint8_t slot, uint8_t function,
                                  uint16_t vendor, uint16_t device, uint8_t class_code,
                                  uint8_t subclass, uint8_t prog_if, uint8_t revision) {
    FakeFunction *fn;
    assert(host->count < FAKE_MAX_FUNCTIONS);
    fn = &host->functions[host->count++];
    memset(fn, 0, sizeof(*fn));
    fn->bus = bus; fn->slot = slot; fn->function = function; fn->present = 1;
    fn->cfg[0] = ((uint32_t)device << 16) | vendor;
    fn->cfg[2] = ((uint32_t)class_code << 24) | ((uint32_t)subclass << 16) |
                 ((uint32_t)prog_if << 8) | revision;
    return fn;
}

static void make_backend(FakeHost *host, BauraPciPortBackend *backend) {
    BauraPciPortIo io;
    io.out32 = fake_out32; io.in32 = fake_in32; io.context = host;
    assert(baura_pci_port_backend_init(backend, &io) == BAURA_OK);
}

static void test_address_and_validation(void) {
    FakeHost host;
    BauraPciPortBackend backend;
    BauraPciPortIo io;
    uint32_t value = 0;

    assert(baura_pci_config_address(0x12, 0x1F, 7, 0xFC) == 0x8012FFFCu);
    assert(baura_pci_config_address(0, 0, 0, 0) == 0x80000000u);
    assert(baura_pci_config_address(0, 3, 0, 0x03) == (0x80000000u | (3u << 11)));

    memset(&host, 0, sizeof(host));
    add_function(&host, 1, 2, 3, 0x8086, 0x100E, 0x02, 0x00, 0x00, 0x03)->cfg[4] = 0xFEBF0000u;
    make_backend(&host, &backend);

    assert(baura_pci_port_config_read(1, 2, 3, 0x10, &value, &backend) == 1);
    assert(value == 0xFEBF0000u);
    assert(host.address == baura_pci_config_address(1, 2, 3, 0x10));
    assert(baura_pci_port_config_read(1, 2, 3, 0x00, &value, &backend) == 1);
    assert(value == 0x100E8086u);
    /* an absent function reads as all ones */
    assert(baura_pci_port_config_read(1, 2, 4, 0x00, &value, &backend) == 1);
    assert(value == 0xFFFFFFFFu);

    value = 0x55555555u;
    assert(baura_pci_port_config_read(0, 32, 0, 0, &value, &backend) == 0);   /* slot out of range */
    assert(baura_pci_port_config_read(0, 0, 8, 0, &value, &backend) == 0);    /* function out of range */
    assert(baura_pci_port_config_read(0, 0, 0, 2, &value, &backend) == 0);    /* not dword-aligned */
    assert(baura_pci_port_config_read(0, 0, 0, 0, 0, &backend) == 0);         /* null value */
    assert(baura_pci_port_config_read(0, 0, 0, 0, &value, 0) == 0);           /* null context */
    assert(value == 0x55555555u);                                             /* untouched on failure */

    io.out32 = 0; io.in32 = fake_in32; io.context = &host;
    assert(baura_pci_port_backend_init(&backend, &io) == BAURA_ERR_INVALID_ARGUMENT);
    assert(baura_pci_port_backend_init(0, &io) == BAURA_ERR_INVALID_ARGUMENT);
    assert(baura_pci_port_backend_init(&backend, 0) == BAURA_ERR_INVALID_ARGUMENT);
}

static void test_port_errors_and_reentrancy(void) {
    FakeHost host;
    BauraPciPortBackend backend;
    uint32_t value = 0x77u;

    memset(&host, 0, sizeof(host));
    add_function(&host, 0, 1, 0, 0x8086, 0x1237, 0x06, 0x00, 0x00, 0x02);
    make_backend(&host, &backend);

    host.fail_out = 1;
    assert(baura_pci_port_config_read(0, 1, 0, 0, &value, &backend) == 0 && value == 0x77u);
    host.fail_out = 0; host.fail_in = 1;
    assert(baura_pci_port_config_read(0, 1, 0, 0, &value, &backend) == 0 && value == 0x77u);
    host.fail_in = 0;
    /* a failure must not leave the backend locked */
    assert(baura_pci_port_config_read(0, 1, 0, 0, &value, &backend) == 1 && value == 0x12378086u);

    host.reenter = &backend; host.reenter_result = 1;
    assert(baura_pci_port_config_read(0, 1, 0, 0, &value, &backend) == 1);
    assert(host.reenter_result == 0);   /* the nested call was refused */
    host.reenter = 0;
    assert(baura_pci_port_config_read(0, 1, 0, 0, &value, &backend) == 1);
}

static void build_machine(FakeHost *host) {
    FakeFunction *fn;
    memset(host, 0, sizeof(*host));
    add_function(host, 0, 0, 0, 0x8086, 0x1237, 0x06, 0x00, 0x00, 0x02);          /* host bridge */
    fn = add_function(host, 0, 3, 0, 0x8086, 0x100E, 0x02, 0x00, 0x00, 0x03);     /* Ethernet */
    fn->cfg[0x3C / 4] = 11;
    fn->cfg[0x2C / 4] = 0x11001AF4u;                                              /* subsystem 1af4:1100 */
    fn->cfg[1] = 0x00100007u;                                                     /* status: capability list */
    fn->cfg[0x34 / 4] = 0x40;
    fn->cfg[0x40 / 4] = 0x00005005u;                                              /* MSI, next 0x50 */
    fn->cfg[0x50 / 4] = 0x00007010u;                                              /* PCIe, next 0x70 */
    fn->cfg[0x70 / 4] = 0x00000011u;                                              /* MSI-X, end */
    add_function(host, 0, 4, 0, 0x144D, 0xA808, 0x01, 0x08, 0x02, 0x01);          /* NVMe */
    fn = add_function(host, 0, 5, 0, 0x8086, 0x1E31, 0x0C, 0x03, 0x30, 0x04);     /* xHCI */
    fn->cfg[3] = 0x00800000u;                                                     /* multi-function */
    add_function(host, 0, 5, 1, 0x8086, 0x1E20, 0x04, 0x03, 0x00, 0x04);          /* HD Audio */
    add_function(host, 0, 6, 0, 0x10EC, 0xB822, 0x0D, 0x80, 0x00, 0x00);          /* wireless */
    add_function(host, 0, 7, 0, 0x1234, 0x5678, 0x09, 0x00, 0x00, 0x00);          /* input controller */
    add_function(host, 0, 8, 0, 0x8086, 0x1E22, 0x0C, 0x05, 0x00, 0x04);          /* SMBus */
    add_function(host, 1, 0, 0, 0x10DE, 0x2684, 0x03, 0x00, 0x00, 0xA1);          /* GPU on bus 1 */
}

static unsigned probe_calls;

static BauraStatus probe_count(BauraDevice *device, void *context) {
    ++probe_calls;
    device->driver_handle = context;
    return BAURA_OK;
}

static void find(uint32_t vendor, uint32_t product, BauraDevice *out) {
    uint32_t index;
    for (index = 0; index < dev_device_count(); ++index) {
        assert(dev_get_device(index, out) == BAURA_OK);
        if (out->vendor_id == vendor && out->product_id == product) return;
    }
    assert(!"device not found");
}

static void test_discovery_and_classification(void) {
    FakeHost host;
    BauraPciPortBackend backend;
    BauraDevice device;
    uint32_t counts[BAURA_DEVICE_TYPE_COUNT] = {0};
    uint32_t index;

    build_machine(&host);
    make_backend(&host, &backend);
    dev_device_manager_init();
    assert(dev_discover_pci() == BAURA_ERR_INVALID_STATE);   /* no reader yet */
    assert(baura_pci_port_backend_install(&backend) == BAURA_OK);
    assert(dev_discover_pci() == BAURA_OK);
    assert(dev_device_count() == 9);

    for (index = 0; index < dev_device_count(); ++index) {
        assert(dev_get_device(index, &device) == BAURA_OK);
        assert((uint32_t)device.type < BAURA_DEVICE_TYPE_COUNT);
        ++counts[device.type];
    }
    assert(counts[DEV_TYPE_STORAGE] == 1 && counts[DEV_TYPE_NETWORK] == 1);
    assert(counts[DEV_TYPE_GPU] == 1 && counts[DEV_TYPE_INPUT] == 1);
    assert(counts[DEV_TYPE_OTHER] == 1 && counts[DEV_TYPE_USB_HOST] == 1);
    assert(counts[DEV_TYPE_BRIDGE] == 1 && counts[DEV_TYPE_AUDIO] == 1);
    assert(counts[DEV_TYPE_WIRELESS] == 1);

    /* a USB host controller and an SMBus controller are not input devices */
    find(0x8086, 0x1E31, &device);
    assert(device.type == DEV_TYPE_USB_HOST && device.function == 0 && device.header_type == 0);
    find(0x8086, 0x1E22, &device);
    assert(device.type == DEV_TYPE_OTHER);
    find(0x1234, 0x5678, &device);
    assert(device.type == DEV_TYPE_INPUT);

    /* multi-function device: function 1 was found */
    find(0x8086, 0x1E20, &device);
    assert(device.slot == 5 && device.function == 1 && device.type == DEV_TYPE_AUDIO);

    /* the GPU is on the second bus */
    find(0x10DE, 0x2684, &device);
    assert(device.bus == 1 && device.type == DEV_TYPE_GPU && device.revision == 0xA1);

    /* subsystem IDs, interrupt line and capabilities */
    find(0x8086, 0x100E, &device);
    assert(device.subsystem_vendor_id == 0x1AF4 && device.subsystem_id == 0x1100);
    assert(device.irq_line == 11);
    assert(device.msi_cap_offset == 0x40 && device.pcie_cap_offset == 0x50 &&
           device.msix_cap_offset == 0x70);
    find(0x144D, 0xA808, &device);
    assert(device.class_code == 0x01 && device.subclass == 0x08 &&
           device.programming_interface == 0x02);
    assert(device.msi_cap_offset == 0 && device.msix_cap_offset == 0 && device.pcie_cap_offset == 0);

    /* identifiers are unique and non-zero */
    for (index = 0; index < dev_device_count(); ++index) {
        BauraDevice other;
        uint32_t second;
        assert(dev_get_device(index, &device) == BAURA_OK && device.device_id != 0);
        for (second = index + 1; second < dev_device_count(); ++second) {
            assert(dev_get_device(second, &other) == BAURA_OK);
            assert(other.device_id != device.device_id);
        }
    }
}

static void test_capability_list_is_bounded(void) {
    FakeHost host;
    BauraPciPortBackend backend;
    BauraDevice device;
    FakeFunction *fn;

    /* A list that points at itself must not hang discovery. */
    memset(&host, 0, sizeof(host));
    fn = add_function(&host, 0, 1, 0, 0x1111, 0x2222, 0x02, 0x00, 0x00, 0x00);
    fn->cfg[1] = 0x00100000u;
    fn->cfg[0x34 / 4] = 0x40;
    fn->cfg[0x40 / 4] = 0x00004001u;   /* power management, next = itself */
    /* A pointer into the standard header is ignored. */
    fn = add_function(&host, 0, 2, 0, 0x3333, 0x4444, 0x02, 0x00, 0x00, 0x00);
    fn->cfg[1] = 0x00100000u;
    fn->cfg[0x34 / 4] = 0x10;
    /* The list is only honoured when the status bit says it exists. */
    fn = add_function(&host, 0, 3, 0, 0x5555, 0x6666, 0x02, 0x00, 0x00, 0x00);
    fn->cfg[0x34 / 4] = 0x40;
    fn->cfg[0x40 / 4] = 0x00000005u;

    make_backend(&host, &backend);
    dev_device_manager_init();
    assert(baura_pci_port_backend_install(&backend) == BAURA_OK);
    assert(dev_discover_pci() == BAURA_OK && dev_device_count() == 3);
    find(0x1111, 0x2222, &device);
    assert(device.msi_cap_offset == 0 && device.msix_cap_offset == 0 && device.pcie_cap_offset == 0);
    find(0x3333, 0x4444, &device);
    assert(device.msi_cap_offset == 0);
    find(0x5555, 0x6666, &device);
    assert(device.msi_cap_offset == 0);
}

static void test_rediscovery_keeps_bindings(void) {
    FakeHost host;
    BauraPciPortBackend backend;
    BauraDevice before, after, other;
    uint32_t count_before;
    uint32_t index;

    build_machine(&host);
    make_backend(&host, &backend);
    dev_device_manager_init();
    assert(baura_pci_port_backend_install(&backend) == BAURA_OK);
    probe_calls = 0;
    assert(dev_register_driver(DEV_TYPE_STORAGE, probe_count, (void *)(uintptr_t)7) == BAURA_OK);
    assert(dev_discover_pci() == BAURA_OK);
    assert(probe_calls == 1);
    find(0x144D, 0xA808, &before);
    assert(before.driver_attached == 1 && before.driver_handle == (void *)(uintptr_t)7);
    count_before = dev_device_count();

    /* scanning again changes nothing for devices that are still there */
    assert(dev_discover_pci() == BAURA_OK);
    find(0x144D, 0xA808, &after);
    assert(dev_device_count() == count_before);
    assert(after.device_id == before.device_id);
    assert(after.driver_attached == 1 && after.driver_handle == (void *)(uintptr_t)7);
    assert(probe_calls == 1);   /* the driver was not probed a second time */

    /* a new device gets a new identifier; the others keep theirs */
    add_function(&host, 0, 9, 0, 0x8086, 0x10D3, 0x02, 0x00, 0x00, 0x00);
    assert(dev_discover_pci() == BAURA_OK);
    assert(dev_device_count() == count_before + 1);
    find(0x8086, 0x10D3, &other);
    for (index = 0; index < dev_device_count(); ++index) {
        BauraDevice device;
        assert(dev_get_device(index, &device) == BAURA_OK);
        assert(device.device_id <= other.device_id);
    }
    find(0x144D, 0xA808, &after);
    assert(after.device_id == before.device_id && after.driver_attached == 1);

    /* a device that disappeared leaves the list */
    host.functions[1].present = 0;   /* the Ethernet controller */
    assert(dev_discover_pci() == BAURA_OK);
    assert(dev_device_count() == count_before);
    for (index = 0; index < dev_device_count(); ++index) {
        BauraDevice device;
        assert(dev_get_device(index, &device) == BAURA_OK);
        assert(!(device.vendor_id == 0x8086 && device.product_id == 0x100E));
    }

    /* a read failure on the whole bus yields an empty list, not a crash */
    host.fail_in = 1;
    assert(dev_discover_pci() == BAURA_OK && dev_device_count() == 0);
}

static void test_registry_limits(void) {
    uint32_t index;
    dev_device_manager_init();
    assert(dev_register_driver((DeviceType)BAURA_DEVICE_TYPE_COUNT, probe_count, 0) ==
           BAURA_ERR_INVALID_ARGUMENT);
    assert(dev_register_driver(DEV_TYPE_WIRELESS, probe_count, 0) == BAURA_OK);
    for (index = 0; index < BAURA_DRIVER_PROBE_CAPACITY - 1u; ++index)
        assert(dev_register_pci_driver(0x1000, (uint16_t)index, probe_count, 0) == BAURA_OK);
    assert(dev_register_pci_driver(0x1000, 0xFFFE, probe_count, 0) == BAURA_ERR_CAPACITY);
    assert(BAURA_DRIVER_PROBE_CAPACITY >= 64u && BAURA_DEVICE_CAPACITY >= 256u);
}

static void test_report_format(void) {
    BauraDevice device;
    char line[BAURA_DEVICE_REPORT_LINE_MAX];
    char small[16];
    memset(&device, 0, sizeof(device));
    device.bus = 0; device.slot = 3; device.function = 0;
    device.vendor_id = 0x8086; device.product_id = 0x100E;
    device.class_code = 0x02; device.subclass = 0x00; device.programming_interface = 0x00;
    device.revision = 0x03; device.type = DEV_TYPE_NETWORK;
    assert(baura_device_format_line(&device, line, sizeof(line)) ==
           strlen("PCI 00:03.0 8086:100e class 02.00.00 rev 03 network\r\n"));
    assert(strcmp(line, "PCI 00:03.0 8086:100e class 02.00.00 rev 03 network\r\n") == 0);

    device.bus = 0xAB; device.slot = 0x1F; device.function = 7;
    device.vendor_id = 0x10DE; device.product_id = 0x2684; device.type = DEV_TYPE_USB_HOST;
    device.class_code = 0x0C; device.subclass = 0x03; device.programming_interface = 0x30;
    device.revision = 0xA1;
    assert(baura_device_format_line(&device, line, sizeof(line)) != 0);
    assert(strcmp(line, "PCI ab:1f.7 10de:2684 class 0c.03.30 rev a1 usb-host\r\n") == 0);

    assert(baura_device_format_line(&device, small, sizeof(small)) == 0);
    assert(baura_device_format_line(0, line, sizeof(line)) == 0);
    assert(baura_device_format_line(&device, 0, sizeof(line)) == 0);
    assert(strcmp(baura_device_type_name((DeviceType)BAURA_DEVICE_TYPE_COUNT), "unknown") == 0);
}

int main(void) {
    test_address_and_validation();
    test_port_errors_and_reentrancy();
    test_discovery_and_classification();
    test_capability_list_is_bounded();
    test_rediscovery_keeps_bindings();
    test_registry_limits();
    test_report_format();
    puts("pci_backend_test: all tests passed");
    return 0;
}
