#include "baura_update_manager.h"

static BauraUpdateState current_state = BAURA_UPDATE_IDLE;
static uint64_t installed_version;

void update_manager_init(void) { current_state = BAURA_UPDATE_IDLE; installed_version = 0; }
void baura_update_set_current_version(uint64_t version) { installed_version = version; }
BauraUpdateState baura_update_state(void) { return current_state; }
uint64_t baura_update_current_version(void) { return installed_version; }

BauraStatus baura_update_apply(uint64_t target_version, BauraUpdateStep verify, BauraUpdateStep apply, BauraUpdateStep rollback, void *context) {
    BauraStatus status;
    if (target_version == 0 || verify == 0 || apply == 0 || rollback == 0) return BAURA_ERR_INVALID_ARGUMENT;
    if (current_state == BAURA_UPDATE_VALIDATING || current_state == BAURA_UPDATE_APPLYING || current_state == BAURA_UPDATE_ROLLING_BACK) return BAURA_ERR_INVALID_STATE;
    if (target_version <= installed_version) return BAURA_ERR_LIMIT;
    current_state = BAURA_UPDATE_VALIDATING;
    status = verify(target_version, context);
    if (status != BAURA_OK) { current_state = BAURA_UPDATE_FAILED; return status; }
    current_state = BAURA_UPDATE_APPLYING;
    status = apply(target_version, context);
    if (status != BAURA_OK) {
        BauraStatus rollback_status;
        current_state = BAURA_UPDATE_ROLLING_BACK;
        rollback_status = rollback(target_version, context);
        current_state = rollback_status == BAURA_OK ? BAURA_UPDATE_FAILED : BAURA_UPDATE_ROLLBACK_FAILED;
        return rollback_status == BAURA_OK ? status : rollback_status;
    }
    installed_version = target_version; current_state = BAURA_UPDATE_COMPLETE;
    return BAURA_OK;
}
