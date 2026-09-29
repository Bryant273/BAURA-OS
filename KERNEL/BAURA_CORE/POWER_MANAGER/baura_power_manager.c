#include "baura_power_manager.h"

static BauraPowerApply apply_profile;
static void *backend_context;
static BauraPowerProfile current_profile = BAURA_POWER_BALANCED;

void power_manager_init(void) { apply_profile = 0; backend_context = 0; current_profile = BAURA_POWER_BALANCED; }

BauraStatus baura_power_set_backend(BauraPowerApply apply, void *context) {
    if (apply == 0) return BAURA_ERR_INVALID_ARGUMENT;
    apply_profile = apply; backend_context = context; return BAURA_OK;
}

BauraStatus baura_power_set_profile(BauraPowerProfile profile) {
    BauraStatus status;
    if ((uint32_t)profile > (uint32_t)BAURA_POWER_EFFICIENT) return BAURA_ERR_INVALID_ARGUMENT;
    if (profile == current_profile) return BAURA_OK;
    if (apply_profile == 0) return BAURA_ERR_INVALID_STATE;
    status = apply_profile(profile, backend_context);
    if (status == BAURA_OK) current_profile = profile;
    return status;
}

BauraPowerProfile baura_power_get_profile(void) { return current_profile; }
