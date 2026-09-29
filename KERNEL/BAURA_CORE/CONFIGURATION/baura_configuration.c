#include "baura_configuration.h"

typedef struct { char key[BAURA_CONFIG_KEY_MAX]; char value[BAURA_CONFIG_VALUE_MAX]; uint8_t active; } ConfigEntry;
static ConfigEntry entries[BAURA_CONFIG_CAPACITY];
static uint32_t entry_count;

static int equal(const char *left, const char *right) {
    uint32_t index = 0;
    if (left == 0 || right == 0) return 0;
    while (left[index] != '\0' && left[index] == right[index]) ++index;
    return left[index] == right[index];
}

static int copy_bounded(char *destination, uint32_t capacity, const char *source) {
    uint32_t index;
    if (source == 0) return 0;
    for (index = 0; index + 1 < capacity && source[index] != '\0'; ++index) destination[index] = source[index];
    if (source[index] != '\0') return 0;
    destination[index] = '\0';
    return 1;
}

void configuration_init(void) {
    uint32_t index;
    for (index = 0; index < BAURA_CONFIG_CAPACITY; ++index) entries[index].active = 0;
    entry_count = 0;
}

BauraStatus baura_config_set(const char *key, const char *value) {
    uint32_t index;
    char key_copy[BAURA_CONFIG_KEY_MAX] = {0};
    char value_copy[BAURA_CONFIG_VALUE_MAX] = {0};
    if (key == 0 || key[0] == '\0' || !copy_bounded(key_copy, BAURA_CONFIG_KEY_MAX, key) || value == 0 || !copy_bounded(value_copy, BAURA_CONFIG_VALUE_MAX, value)) return BAURA_ERR_INVALID_ARGUMENT;
    for (index = 0; index < BAURA_CONFIG_CAPACITY; ++index) if (entries[index].active && equal(entries[index].key, key_copy)) {
        uint32_t j; for (j = 0; j < BAURA_CONFIG_VALUE_MAX; ++j) entries[index].value[j] = value_copy[j]; return BAURA_OK;
    }
    for (index = 0; index < BAURA_CONFIG_CAPACITY; ++index) if (!entries[index].active) {
        uint32_t j; for (j = 0; j < BAURA_CONFIG_KEY_MAX; ++j) entries[index].key[j] = key_copy[j];
        for (j = 0; j < BAURA_CONFIG_VALUE_MAX; ++j) entries[index].value[j] = value_copy[j];
        entries[index].active = 1; ++entry_count; return BAURA_OK;
    }
    return BAURA_ERR_CAPACITY;
}

BauraStatus baura_config_get(const char *key, char *value, uint32_t capacity) {
    uint32_t index;
    uint32_t length = 0;
    if (key == 0 || value == 0 || capacity == 0) return BAURA_ERR_INVALID_ARGUMENT;
    for (index = 0; index < BAURA_CONFIG_CAPACITY; ++index) if (entries[index].active && equal(entries[index].key, key)) {
        while (entries[index].value[length] != '\0') ++length;
        if (length + 1 > capacity) return BAURA_ERR_LIMIT;
        for (length = 0; entries[index].value[length] != '\0'; ++length) value[length] = entries[index].value[length];
        value[length] = '\0'; return BAURA_OK;
    }
    return BAURA_ERR_NOT_FOUND;
}

BauraStatus baura_config_remove(const char *key) {
    uint32_t index;
    if (key == 0) return BAURA_ERR_INVALID_ARGUMENT;
    for (index = 0; index < BAURA_CONFIG_CAPACITY; ++index) if (entries[index].active && equal(entries[index].key, key)) {
        entries[index].active = 0; --entry_count; return BAURA_OK;
    }
    return BAURA_ERR_NOT_FOUND;
}

uint32_t baura_config_count(void) { return entry_count; }
