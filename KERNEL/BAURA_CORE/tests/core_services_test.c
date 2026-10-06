#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "../SYSTEM_MANAGER/baura_system_manager.h"
#include "../CONFIGURATION/baura_configuration.h"
#include "../DEVICE_MANAGER/BAURA_DEVICE_MGR.h"
#include "../DRIVER_MANAGER/baura_driver_manager.h"
#include "../HARDWARE_MANAGER/baura_hardware_manager.h"
#include "../UPDATE_MANAGER/baura_update_manager.h"
#include "../VIRTUALIZATION/baura_virtualization.h"
#include "../EVENT_BUS/baura_event_bus.h"
#include "../FAULT_ENGINE/baura_fault_engine.h"
#include "../RECOVERY_ENGINE/baura_recovery_engine.h"
#include "../LOGGING/baura_logging.h"
#include "../MONITORING/baura_monitoring.h"
#include "../PERFORMANCE/baura_performance.h"
#include "../NETWORK/baura_network.h"
#include "../POWER_MANAGER/baura_power_manager.h"
#include "../SYSTEM_API/baura_system_api.h"
#include "../SYSTEM_API/baura_core_entry.h"
#include "../PROCESS_MANAGER/baura_process_manager.h"
#include "../RESOURCE_MANAGER/baura_resource_manager.h"
#include "../SERVICE_MANAGER/baura_service_manager.h"
#include "../SECURITY/BAURA_SECURITY_RUST.h"
#include "../STORAGE/baura_storage.h"
#include "../STORAGE/VOLUME_MANAGER/volume_manager.h"

_Static_assert(sizeof(BauraSize) == 8, "BAURA_SIZE must be 64-bit");
_Static_assert(sizeof(BauraOffset) == 8, "BAURA_OFFSET must be 64-bit");
_Static_assert(sizeof(BauraFileHandle) == 8, "BAURA_FILE_HANDLE must be 64-bit");
#if defined(BAURA_TARGET_X86_64)
_Static_assert(sizeof(void *) == 8, "x86_64 Core builds require 64-bit pointers");
#endif
_Static_assert(BAURA_ABI_STATUS_SUCCESS == 0, "ABI success value must remain stable");
_Static_assert(BAURA_ABI_STATUS_PENDING == 2, "ABI pending value must remain stable");
_Static_assert(BAURA_ABI_ERR_INVALID_ARGUMENT == 0x1001,
               "ABI error values must remain stable");
_Static_assert(BAURA_ABI_ERR_INTERNAL == 0x100E,
               "ABI error values must remain stable");
_Static_assert(BAURA_IPC_HEADER_SIZE == 32u, "IPC header size must remain stable");
_Static_assert(BAURA_IPC_MAGIC == UINT32_C(0x52554142), "IPC magic must remain stable");
_Static_assert(BAURA_IPC_MAX_PAYLOAD == 16384u, "IPC payload limit must remain stable");
_Static_assert(BAURA_IPC_REQUEST_ID_OFFSET == 16u, "IPC header offsets must remain stable");
_Static_assert(BAURA_ABI_OPCODE_FS_CREATE == UINT32_C(0x00010001),
               "FS create opcode must remain stable");
_Static_assert(BAURA_ABI_OPCODE_FS_READ == UINT32_C(0x00010002),
               "FS read opcode must remain stable");
_Static_assert(BAURA_ABI_OPCODE_PROCESS_CREATE == UINT32_C(0x00020001),
               "Process create opcode must remain stable");
_Static_assert(BAURA_ABI_OPCODE_MEMORY_ALLOCATE == UINT32_C(0x00030001),
               "Memory allocate opcode must remain stable");
_Static_assert(BAURA_ABI_OPCODE_NETWORK_CONNECT == UINT32_C(0x00040001),
               "Network connect opcode must remain stable");

static uint32_t events_seen;
static uint32_t starts_seen;
static uint32_t stops_seen;
static uint32_t recoveries_seen;
static uint8_t memory_disk[4][16];
static uint32_t frames_sent;
static BauraPowerProfile applied_power;
static uint32_t rollbacks_seen;
static uint32_t destroyed_vms;
static uint8_t updates_succeed;
static char console_output[32];
static size_t console_output_length;

static BauraStatus console_write_fixture(void *context, const char *text, size_t length) {
    size_t index;
    size_t *written = (size_t *)context;
    if (length > sizeof(console_output)) return BAURA_ERR_LIMIT;
    for (index = 0; index < length; ++index) console_output[index] = text[index];
    *written = length;
    return BAURA_OK;
}

static void on_event(const BauraEvent *event, void *context) {
    uint32_t *value = (uint32_t *)context;
    assert(event->topic == 7);
    ++*value;
}

static BauraStatus transmit_fixture(void *context, const uint8_t *frame, uint32_t length) {
    uint32_t *sent = (uint32_t *)context;
    assert(frame[0] == 0xAA); *sent = length; return BAURA_OK;
}
static BauraStatus power_fixture(BauraPowerProfile profile, void *context) {
    BauraPowerProfile *applied = (BauraPowerProfile *)context;
    if (profile == BAURA_POWER_EFFICIENT) return BAURA_ERR_LIMIT;
    *applied = profile; return BAURA_OK;
}
static BauraStatus update_verify(uint64_t version, void *context) { (void)version; (void)context; return BAURA_OK; }
static BauraStatus update_apply(uint64_t version, void *context) { (void)version; return *(uint8_t *)context ? BAURA_OK : BAURA_ERR_INVALID_STATE; }
static BauraStatus update_rollback(uint64_t version, void *context) { (void)version; (void)context; ++rollbacks_seen; return BAURA_OK; }
static BauraStatus vm_probe(void *context, uint8_t *available) { *available = *(uint8_t *)context; return BAURA_OK; }
static BauraStatus vm_create(const BauraVmConfig *config, void *context, uint64_t *id) { (void)config; (void)context; *id = 900; return BAURA_OK; }
static BauraStatus vm_destroy(uint64_t id, void *context) { (void)context; assert(id == 900); ++destroyed_vms; return BAURA_OK; }

static BauraStatus memory_read(void *context, uint64_t lba, uint32_t count, void *buffer) {
    uint8_t (*disk)[16] = (uint8_t (*)[16])context;
    memcpy(buffer, &disk[lba][0], count * 16); return BAURA_OK;
}

static BauraStatus memory_write(void *context, uint64_t lba, uint32_t count, const void *buffer) {
    uint8_t (*disk)[16] = (uint8_t (*)[16])context;
    memcpy(&disk[lba][0], buffer, count * 16); return BAURA_OK;
}

static int pci_fixture(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset, uint32_t *value, void *context) {
    (void)context;
    if (bus != 0 || slot != 5 || function != 0) return 0;
    if (offset == 0x00) *value = 0x12348086u;
    else if (offset == 0x08) *value = 0x01080001u;
    else if (offset == 0x0C) *value = 0;
    else if (offset == 0x10) *value = 0xFEBF0000u;
    else if (offset == 0x3C) *value = 11;
    else return 0;
    return 1;
}

static BauraStatus probe_storage(BauraDevice *device, void *context) {
    device->driver_handle = context;
    return BAURA_OK;
}

static BauraStatus recover_fault(const BauraFaultRecord *fault, void *context) {
    (void)context; assert(fault->code == 55); ++recoveries_seen; return BAURA_OK;
}

static BauraStatus service_start(void *context) {
    (void)context; ++starts_seen; return BAURA_OK;
}

static BauraStatus service_stop(void *context) {
    (void)context; ++stops_seen; return BAURA_OK;
}

static void test_system_and_configuration(void) {
    char value[16];
    BauraCorePlatform platform = {console_write_fixture, &console_output_length};
    static const char expected_console[] = "BAURA OS Core ready\r\n";
    assert(BAURA_ABI_VERSION_MAJOR == 2u && BAURA_ABI_VERSION_MINOR == 0u);
    assert(strcmp(BAURA_ABI_OP_FS_CREATE, "FS-0001") == 0);
    assert(strcmp(BAURA_ABI_OP_FS_READ, "FS-0002") == 0);
    assert(strcmp(BAURA_ABI_OP_PROCESS_CREATE, "PR-0001") == 0);
    assert(strcmp(BAURA_ABI_OP_MEMORY_ALLOCATE, "MEM-0001") == 0);
    assert(strcmp(BAURA_ABI_OP_NETWORK_CONNECT, "NET-0001") == 0);
    assert(strcmp(BAURA_ABI_SYMBOL_NETWORK_CONNECT, "BAURA_NET_CONNECT") == 0);
    assert(BAURA_FS_CREATE_NEW == UINT64_C(1));
    assert(BAURA_FS_CREATE_ALWAYS == UINT64_C(2));
    console_output_length = 0;
    assert(baura_core_initial_task_start(0) == BAURA_ERR_INVALID_ARGUMENT);
    assert(baura_core_initial_task_start(&platform) == BAURA_OK);
    assert(console_output_length == sizeof(expected_console) - 1u);
    assert(memcmp(console_output, expected_console, console_output_length) == 0);
    assert(baura_core_initial_task_start(&platform) == BAURA_ERR_INVALID_STATE);
    assert(baura_system_start() == BAURA_OK);
    assert(baura_system_state() == BAURA_SYSTEM_READY);
    assert(baura_config_set("user.locale", "") == BAURA_OK);
    assert(baura_config_get("user.locale", value, sizeof(value)) == BAURA_OK);
    assert(value[0] == '\0');
    assert(baura_config_set("user.locale", "fr-FR") == BAURA_OK);
    assert(baura_config_get("user.locale", value, sizeof(value)) == BAURA_OK);
    assert(value[0] == 'f' && value[5] == '\0');
    assert(baura_config_remove("user.locale") == BAURA_OK);
    assert(baura_api_config_set("system.mode", "test") == BAURA_OK);
    assert(baura_api_config_set("system.mode", "test") == BAURA_OK);
}

static void test_events_and_logs(void) {
    uint32_t token;
    uint32_t i;
    BauraEvent event = {7, 1, 0, {0}};
    BauraLogRecord record;
    events_seen = 0;
    assert(baura_event_subscribe(7, on_event, &events_seen, &token) == BAURA_OK);
    assert(baura_event_publish(&event) == BAURA_OK && events_seen == 1);
    assert(baura_event_unsubscribe(token) == BAURA_OK);
    assert(baura_event_publish(&event) == BAURA_OK && events_seen == 1);
    for (i = 0; i < BAURA_LOG_CAPACITY + 3; ++i) assert(baura_log_write(BAURA_LOG_INFO, "test") == BAURA_OK);
    assert(baura_log_count() == BAURA_LOG_CAPACITY);
    assert(baura_log_read(0, &record) == BAURA_OK);
    assert(record.sequence == 5);
}

static void test_services_and_processes(void) {
    uint32_t service_id;
    uint64_t parent_id;
    uint64_t child_id;
    BauraProcessInfo process;
    starts_seen = 0; stops_seen = 0;
    assert(baura_service_register("storage", service_start, service_stop, 0, &service_id) == BAURA_OK);
    assert(baura_service_start(service_id) == BAURA_OK && starts_seen == 1);
    assert(baura_service_stop(service_id) == BAURA_OK && stops_seen == 1);
    assert(baura_process_create("init", 0, &parent_id) == BAURA_OK);
    assert(baura_process_create("shell", parent_id, &child_id) == BAURA_OK);
    assert(baura_process_set_state(parent_id, BAURA_PROCESS_BLOCKED) == BAURA_ERR_INVALID_STATE);
    assert(baura_process_set_state(parent_id, BAURA_PROCESS_RUNNING) == BAURA_OK);
    assert(baura_process_set_state(parent_id, BAURA_PROCESS_TERMINATED) == BAURA_OK);
    assert(baura_process_query(child_id, &process) == BAURA_OK);
    assert(process.parent_id == parent_id && process.state == BAURA_PROCESS_READY);
}

static void test_metrics_and_counters(void) {
    BauraMetricSample sample;
    uint64_t counter;
    assert(baura_monitor_record(1, 100, 42) == BAURA_OK);
    assert(baura_monitor_record(1, 101, 45) == BAURA_OK);
    assert(baura_monitor_read_latest(1, 0, &sample) == BAURA_OK);
    assert(sample.timestamp == 101 && sample.value == 45);
    assert(baura_counter_set(1, UINT64_MAX) == BAURA_OK);
    assert(baura_counter_add(1, 1) == BAURA_ERR_LIMIT);
    assert(baura_counter_read(1, &counter) == BAURA_OK && counter == UINT64_MAX);
}

static void test_fault_recovery(void) {
    uint64_t fault_id;
    BauraFaultRecord fault;
    recoveries_seen = 0;
    assert(baura_recovery_register(55, recover_fault, 0) == BAURA_OK);
    assert(baura_fault_report(55, 2, 123, BAURA_FAULT_RECOVERABLE, &fault_id) == BAURA_OK);
    assert(baura_fault_read(0, &fault) == BAURA_OK && fault.id == fault_id);
    assert(baura_recovery_execute(&fault) == BAURA_OK && recoveries_seen == 1);
}

static void test_volume_storage(void) {
    uint8_t input[32];
    uint8_t output[32];
    uint32_t volume_id;
    BauraBlockDevice device = {memory_disk, 4, 16, memory_read, memory_write};
    memset(memory_disk, 0, sizeof(memory_disk));
    memset(input, 0xA5, sizeof(input));
    assert(baura_volume_register("C", 1, &device) == BAURA_OK);
    assert(baura_volume_get_id("C", &volume_id) == BAURA_OK && volume_id == 1);
    assert(baura_storage_write(volume_id, 1, 2, input) == BAURA_OK);
    assert(baura_storage_read(volume_id, 1, 2, output) == BAURA_OK);
    assert(memcmp(input, output, sizeof(input)) == 0);
    assert(baura_storage_read(volume_id, 3, 2, output) == BAURA_ERR_LIMIT);
}

static void test_network_and_power(void) {
    uint8_t frame[32] = {0xAA};
    frames_sent = 0; applied_power = BAURA_POWER_BALANCED;
    assert(baura_network_send_frame(frame, 14) == BAURA_ERR_INVALID_STATE);
    assert(baura_network_set_backend(transmit_fixture, &frames_sent, 20) == BAURA_OK);
    assert(baura_network_send_frame(frame, 14) == BAURA_OK && frames_sent == 14);
    assert(baura_network_send_frame(frame, 21) == BAURA_ERR_LIMIT);
    assert(baura_power_set_profile(BAURA_POWER_PERFORMANCE) == BAURA_ERR_INVALID_STATE);
    assert(baura_power_set_backend(power_fixture, &applied_power) == BAURA_OK);
    assert(baura_power_set_profile(BAURA_POWER_PERFORMANCE) == BAURA_OK);
    assert(baura_power_get_profile() == BAURA_POWER_PERFORMANCE);
    assert(baura_power_set_profile(BAURA_POWER_EFFICIENT) == BAURA_ERR_LIMIT);
    assert(baura_power_get_profile() == BAURA_POWER_PERFORMANCE);
}

static void test_hardware_update_virtualization(void) {
    BauraHardwareSummary summary;
    uint8_t available = 1;
    uint64_t vm_id;
    BauraVmConfig config = {64u * 1024u * 1024u, 2};
    assert(baura_hardware_get_summary(&summary) == BAURA_OK);
    assert(summary.total == 1 && summary.by_type[DEV_TYPE_STORAGE] == 1);
    baura_update_set_current_version(1);
    updates_succeed = 1; rollbacks_seen = 0;
    assert(baura_update_apply(2, update_verify, update_apply, update_rollback, &updates_succeed) == BAURA_OK);
    assert(baura_update_current_version() == 2 && baura_update_state() == BAURA_UPDATE_COMPLETE);
    updates_succeed = 0;
    assert(baura_update_apply(3, update_verify, update_apply, update_rollback, &updates_succeed) == BAURA_ERR_INVALID_STATE);
    assert(baura_update_current_version() == 2 && rollbacks_seen == 1);
    destroyed_vms = 0;
    assert(baura_virtualization_set_provider(vm_probe, vm_create, vm_destroy, &available) == BAURA_OK);
    assert(baura_vm_create(&config, &vm_id) == BAURA_OK && vm_id == 900);
    assert(baura_vm_count() == 1);
    assert(baura_vm_destroy(vm_id) == BAURA_OK && destroyed_vms == 1);
    assert(baura_vm_count() == 0);
}

static void test_pci_registry(void) {
    BauraDevice device;
    assert(dev_set_pci_reader(pci_fixture, 0) == BAURA_OK);
    assert(baura_driver_register(DEV_TYPE_STORAGE, probe_storage, (void *)(uintptr_t)1) == BAURA_OK);
    assert(baura_hardware_refresh() == BAURA_OK);
    assert(dev_device_count() == 1);
    assert(dev_get_device(0, &device) == BAURA_OK);
    assert(device.vendor_id == 0x8086 && device.product_id == 0x1234);
    assert(device.type == DEV_TYPE_STORAGE && device.irq_line == 11);
    assert(device.driver_attached == 1 && device.driver_handle == (void *)(uintptr_t)1);
}

static void test_capability_security(void) {
    assert(sec_create_cspace(71) == 0);
    assert(sec_create_cspace(72) == 0);
    assert(sec_mint_capability(71, 1, 99, CAP_READ | CAP_WRITE | CAP_GRANT) == 0);
    assert(sec_delegate_capability(71, 1, 72, 2, CAP_READ) == 0);
    assert(sec_validate_capability(72, 2, CAP_READ) == 1);
    assert(sec_validate_capability(72, 2, CAP_WRITE) == 0);
    assert(sec_delegate_capability(71, 1, 72, 3, CAP_EXEC) == -6);
    assert(sec_revoke_capability(72, 2) == 0);
    assert(sec_validate_capability(72, 2, CAP_READ) == 0);
    assert(sec_destroy_cspace(71) == 0 && sec_destroy_cspace(72) == 0);
}

static void test_resource_quotas(void) {
    BauraResourceUsage usage;
    assert(baura_resource_reserve(44, 1) == BAURA_ERR_LIMIT);
    assert(baura_resource_set_limit(44, 10) == BAURA_OK);
    assert(baura_resource_reserve(44, 8) == BAURA_OK);
    assert(baura_resource_reserve(44, 3) == BAURA_ERR_LIMIT);
    assert(baura_resource_query(44, &usage) == BAURA_OK);
    assert(usage.used == 8 && usage.limit == 10);
    assert(baura_resource_release(44, 8) == BAURA_OK);
}

int main(void) {
    test_system_and_configuration();
    test_events_and_logs();
    test_services_and_processes();
    test_metrics_and_counters();
    test_fault_recovery();
    test_volume_storage();
    test_pci_registry();
    test_network_and_power();
    test_hardware_update_virtualization();
    test_resource_quotas();
    test_capability_security();
    return 0;
}
