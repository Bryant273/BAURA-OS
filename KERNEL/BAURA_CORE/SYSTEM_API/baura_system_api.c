#include "baura_system_api.h"
#include "../CONFIGURATION/baura_configuration.h"

static uint8_t api_ready;

void system_api_init(void) { api_ready = 1; }

BauraStatus baura_api_log(BauraLogLevel level, const char *message) {
    if (!api_ready) return BAURA_ERR_INVALID_STATE;
    return baura_log_write(level, message);
}

BauraStatus baura_api_process_query(uint64_t process_id, BauraProcessInfo *info) {
    if (!api_ready) return BAURA_ERR_INVALID_STATE;
    return baura_process_query(process_id, info);
}

BauraStatus baura_api_config_get(const char *key, char *value, uint32_t capacity) {
    if (!api_ready) return BAURA_ERR_INVALID_STATE;
    return baura_config_get(key, value, capacity);
}

BauraStatus baura_api_config_set(const char *key, const char *value) {
    if (!api_ready) return BAURA_ERR_INVALID_STATE;
    return baura_config_set(key, value);
}
