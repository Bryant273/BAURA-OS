#include "baura_performance.h"

typedef struct { uint32_t id; uint64_t value; uint8_t active; } Counter;
static Counter counters[BAURA_COUNTER_CAPACITY];

static Counter *find_counter(uint32_t id) {
    uint32_t index;
    for (index = 0; index < BAURA_COUNTER_CAPACITY; ++index) if (counters[index].active && counters[index].id == id) return &counters[index];
    return 0;
}

void performance_init(void) {
    uint32_t index;
    for (index = 0; index < BAURA_COUNTER_CAPACITY; ++index) counters[index].active = 0;
}

BauraStatus baura_counter_set(uint32_t counter_id, uint64_t value) {
    uint32_t index;
    Counter *counter;
    if (counter_id == 0) return BAURA_ERR_INVALID_ARGUMENT;
    counter = find_counter(counter_id);
    if (counter != 0) { counter->value = value; return BAURA_OK; }
    for (index = 0; index < BAURA_COUNTER_CAPACITY; ++index) if (!counters[index].active) {
        counters[index].id = counter_id; counters[index].value = value; counters[index].active = 1; return BAURA_OK;
    }
    return BAURA_ERR_CAPACITY;
}

BauraStatus baura_counter_add(uint32_t counter_id, uint64_t increment) {
    uint64_t value;
    Counter *counter = find_counter(counter_id);
    if (counter == 0) return BAURA_ERR_NOT_FOUND;
    if (UINT64_MAX - counter->value < increment) return BAURA_ERR_LIMIT;
    value = counter->value + increment; counter->value = value; return BAURA_OK;
}

BauraStatus baura_counter_read(uint32_t counter_id, uint64_t *value) {
    Counter *counter;
    if (counter_id == 0 || value == 0) return BAURA_ERR_INVALID_ARGUMENT;
    counter = find_counter(counter_id);
    if (counter == 0) return BAURA_ERR_NOT_FOUND;
    *value = counter->value; return BAURA_OK;
}
