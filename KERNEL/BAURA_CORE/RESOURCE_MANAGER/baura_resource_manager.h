#ifndef BAURA_RESOURCE_MANAGER_H
#define BAURA_RESOURCE_MANAGER_H

#include "../baura_core_types.h"

#define BAURA_RESOURCE_OWNER_CAPACITY 32u

typedef struct {
    uint64_t limit;
    uint64_t used;
} BauraResourceUsage;

void resource_manager_init(void);
BauraStatus baura_resource_set_limit(uint64_t owner_id, uint64_t limit);
BauraStatus baura_resource_reserve(uint64_t owner_id, uint64_t amount);
BauraStatus baura_resource_release(uint64_t owner_id, uint64_t amount);
BauraStatus baura_resource_query(uint64_t owner_id, BauraResourceUsage *usage);
uint32_t baura_resource_owner_count(void);

#endif
