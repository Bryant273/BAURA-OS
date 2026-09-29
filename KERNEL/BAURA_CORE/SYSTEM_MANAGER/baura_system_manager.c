#include "baura_system_manager.h"
#include "../CONFIGURATION/baura_configuration.h"
#include "../EVENT_BUS/baura_event_bus.h"
#include "../HARDWARE_MANAGER/baura_hardware_manager.h"
#include "../UPDATE_MANAGER/baura_update_manager.h"
#include "../VIRTUALIZATION/baura_virtualization.h"
#include "../DEVICE_MANAGER/baura_device_manager.h"
#include "../DRIVER_MANAGER/baura_driver_manager.h"
#include "../FAULT_ENGINE/baura_fault_engine.h"
#include "../RECOVERY_ENGINE/baura_recovery_engine.h"
#include "../LOGGING/baura_logging.h"
#include "../MONITORING/baura_monitoring.h"
#include "../PERFORMANCE/baura_performance.h"
#include "../PROCESS_MANAGER/baura_process_manager.h"
#include "../RESOURCE_MANAGER/baura_resource_manager.h"
#include "../SERVICE_MANAGER/baura_service_manager.h"
#include "../SECURITY/baura_security.h"
#include "../STORAGE/baura_storage.h"
#include "../STORAGE/VOLUME_MANAGER/volume_manager.h"
#include "../NETWORK/baura_network.h"
#include "../POWER_MANAGER/baura_power_manager.h"
#include "../SYSTEM_API/baura_system_api.h"

static BauraSystemState current_state = BAURA_SYSTEM_COLD;

BauraStatus baura_system_start(void) {
    if (current_state == BAURA_SYSTEM_READY) return BAURA_OK;
    if (current_state != BAURA_SYSTEM_COLD && current_state != BAURA_SYSTEM_FAILED) return BAURA_ERR_INVALID_STATE;
    current_state = BAURA_SYSTEM_INITIALIZING;
    logging_init();
    monitoring_init();
    performance_init();
    event_bus_init();
    device_manager_init();
    hardware_manager_init();
    driver_manager_init();
    fault_engine_init();
    recovery_engine_init();
    resource_manager_init();
    service_manager_init();
    security_init();
    configuration_init();
    process_manager_init();
    update_manager_init();
    virtualization_init();
    network_init();
    power_manager_init();
    system_api_init();
    volume_manager_init();
    storage_init();
    current_state = BAURA_SYSTEM_READY;
    baura_log_write(BAURA_LOG_INFO, "BAURA Core initialized");
    return BAURA_OK;
}

void system_manager_init(void) { (void)baura_system_start(); }

BauraStatus baura_system_shutdown(void) {
    if (current_state != BAURA_SYSTEM_READY) return BAURA_ERR_INVALID_STATE;
    current_state = BAURA_SYSTEM_SHUTTING_DOWN;
    current_state = BAURA_SYSTEM_COLD;
    return BAURA_OK;
}

BauraSystemState baura_system_state(void) { return current_state; }
