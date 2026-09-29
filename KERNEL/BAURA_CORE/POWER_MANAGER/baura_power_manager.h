#ifndef BAURA_POWER_MANAGER_H
#define BAURA_POWER_MANAGER_H

#include "../baura_core_types.h"

typedef enum { BAURA_POWER_BALANCED, BAURA_POWER_PERFORMANCE, BAURA_POWER_EFFICIENT } BauraPowerProfile;
typedef BauraStatus (*BauraPowerApply)(BauraPowerProfile profile, void *context);

void power_manager_init(void);
BauraStatus baura_power_set_backend(BauraPowerApply apply, void *context);
BauraStatus baura_power_set_profile(BauraPowerProfile profile);
BauraPowerProfile baura_power_get_profile(void);

#endif
