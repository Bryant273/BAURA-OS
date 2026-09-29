#ifndef BAURA_SYSTEM_MANAGER_H
#define BAURA_SYSTEM_MANAGER_H

#include "../baura_core_types.h"

typedef enum { BAURA_SYSTEM_COLD, BAURA_SYSTEM_INITIALIZING, BAURA_SYSTEM_READY, BAURA_SYSTEM_FAILED, BAURA_SYSTEM_SHUTTING_DOWN } BauraSystemState;

void system_manager_init(void);
BauraStatus baura_system_start(void);
BauraStatus baura_system_shutdown(void);
BauraSystemState baura_system_state(void);

#endif
