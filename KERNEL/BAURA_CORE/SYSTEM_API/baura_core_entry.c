#include "baura_core_entry.h"
#include "../SYSTEM_MANAGER/baura_system_manager.h"

static const char ready_message[] = "BAURA OS Core ready\r\n";

BauraStatus baura_core_initial_task_start(const BauraCorePlatform *platform) {
    BauraStatus status;
    if (platform == 0 || platform->console_write == 0) return BAURA_ERR_INVALID_ARGUMENT;
    if (baura_system_state() != BAURA_SYSTEM_COLD) return BAURA_ERR_INVALID_STATE;

    status = baura_system_start();
    if (status != BAURA_OK) return status;
    return platform->console_write(platform->console_context, ready_message,
                                   sizeof(ready_message) - 1u);
}
