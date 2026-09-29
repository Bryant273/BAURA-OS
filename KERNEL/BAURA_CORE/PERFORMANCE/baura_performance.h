#ifndef BAURA_PERFORMANCE_H
#define BAURA_PERFORMANCE_H

#include "../baura_core_types.h"

#define BAURA_COUNTER_CAPACITY 32u

void performance_init(void);
BauraStatus baura_counter_set(uint32_t counter_id, uint64_t value);
BauraStatus baura_counter_add(uint32_t counter_id, uint64_t increment);
BauraStatus baura_counter_read(uint32_t counter_id, uint64_t *value);

#endif
