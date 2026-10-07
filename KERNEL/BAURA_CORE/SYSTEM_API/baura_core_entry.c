#include "baura_core_entry.h"
#include "../SYSTEM_MANAGER/baura_system_manager.h"

static const char ready_message[] = "BAURA OS Core ready\r\n";

BauraStatus baura_core_initial_task_start(const BauraCorePlatform *platform) {
    BauraStatus status;
    const int has_graphical_extension =
        platform != 0 &&
        platform->struct_size >=
            offsetof(BauraCorePlatform, graphical_service_context) +
                sizeof(platform->graphical_service_context);
    if (platform == 0) return BAURA_ERR_INVALID_ARGUMENT;
    if (platform->abi_version != BAURA_CORE_PLATFORM_ABI_VERSION ||
        platform->struct_size <
            offsetof(BauraCorePlatform, start_graphical_service) ||
        (platform->struct_size > offsetof(BauraCorePlatform, start_graphical_service) &&
         platform->struct_size < sizeof(*platform)) ||
        platform->boot_info == 0 ||
        platform->validate_boot_info == 0 || platform->console_write == 0) {
        return BAURA_ERR_INVALID_ARGUMENT;
    }
    if (!has_graphical_extension && platform->struct_size !=
                                       offsetof(BauraCorePlatform,
                                                start_graphical_service)) {
        return BAURA_ERR_INVALID_ARGUMENT;
    }
    if (baura_system_state() != BAURA_SYSTEM_COLD) return BAURA_ERR_INVALID_STATE;
    if ((platform->process_backend.start != 0 ||
         platform->process_backend.set_state != 0 ||
         platform->process_backend.terminate != 0) &&
        (platform->process_backend.start == 0 ||
         platform->process_backend.set_state == 0 ||
         platform->process_backend.terminate == 0)) {
        return BAURA_ERR_INVALID_ARGUMENT;
    }
    if (has_graphical_extension && platform->start_graphical_service == 0 &&
        platform->graphical_service_context != 0) {
        return BAURA_ERR_INVALID_ARGUMENT;
    }

    status = platform->validate_boot_info(platform->boot_info_context,
                                          platform->boot_info);
    if (status != BAURA_OK) return status;

    status = baura_process_set_backend(
        platform->process_backend.start == 0 ? 0 : &platform->process_backend);
    if (status != BAURA_OK) return status;
    status = baura_security_set_authority_check(platform->authority_check,
                                                platform->authority_context);
    if (status != BAURA_OK) return status;
    status = baura_system_start();
    if (status != BAURA_OK) return status;
    if (has_graphical_extension && platform->start_graphical_service != 0) {
        status = platform->start_graphical_service(
            platform->graphical_service_context);
        if (status != BAURA_OK) return status;
    }
    return platform->console_write(platform->console_context, ready_message,
                                   sizeof(ready_message) - 1u);
}
