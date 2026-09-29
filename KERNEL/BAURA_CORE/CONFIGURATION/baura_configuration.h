#ifndef BAURA_CONFIGURATION_H
#define BAURA_CONFIGURATION_H

#include "../baura_core_types.h"

#define BAURA_CONFIG_CAPACITY 64u
#define BAURA_CONFIG_KEY_MAX 32u
#define BAURA_CONFIG_VALUE_MAX 96u

void configuration_init(void);
BauraStatus baura_config_set(const char *key, const char *value);
BauraStatus baura_config_get(const char *key, char *value, uint32_t capacity);
BauraStatus baura_config_remove(const char *key);
uint32_t baura_config_count(void);

#endif
