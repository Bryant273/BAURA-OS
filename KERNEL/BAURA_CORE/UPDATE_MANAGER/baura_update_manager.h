#ifndef BAURA_UPDATE_MANAGER_H
#define BAURA_UPDATE_MANAGER_H

#include "../baura_core_types.h"

typedef enum { BAURA_UPDATE_IDLE, BAURA_UPDATE_VALIDATING, BAURA_UPDATE_APPLYING, BAURA_UPDATE_ROLLING_BACK, BAURA_UPDATE_COMPLETE, BAURA_UPDATE_FAILED, BAURA_UPDATE_ROLLBACK_FAILED } BauraUpdateState;
typedef BauraStatus (*BauraUpdateStep)(uint64_t target_version, void *context);

void update_manager_init(void);
void baura_update_set_current_version(uint64_t version);
BauraStatus baura_update_apply(uint64_t target_version, BauraUpdateStep verify, BauraUpdateStep apply, BauraUpdateStep rollback, void *context);
BauraUpdateState baura_update_state(void);
uint64_t baura_update_current_version(void);

#endif
