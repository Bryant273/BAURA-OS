#ifndef BAURA_SYSTEM_API_H
#define BAURA_SYSTEM_API_H

#include "../baura_core_types.h"
#include "baura_abi_v2.h"
#include "../LOGGING/baura_logging.h"
#include "../PROCESS_MANAGER/baura_process_manager.h"

void system_api_init(void);
BauraStatus baura_api_log(BauraLogLevel level, const char *message);
BauraStatus baura_api_process_query(uint64_t process_id, BauraProcessInfo *info);
BauraStatus baura_api_config_get(const char *key, char *value, uint32_t capacity);
BauraStatus baura_api_config_set(const char *key, const char *value);

#endif
