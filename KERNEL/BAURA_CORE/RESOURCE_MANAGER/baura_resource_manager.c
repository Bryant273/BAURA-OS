#include "baura_resource_manager.h"

typedef struct {
    uint64_t owner_id;
    BauraResourceUsage usage;
    uint8_t active;
} ResourceOwner;

static ResourceOwner owners[BAURA_RESOURCE_OWNER_CAPACITY];
static uint32_t owner_count;

static ResourceOwner *find_owner(uint64_t owner_id) {
    uint32_t index;
    for (index = 0; index < BAURA_RESOURCE_OWNER_CAPACITY; ++index)
        if (owners[index].active && owners[index].owner_id == owner_id) return &owners[index];
    return 0;
}

static ResourceOwner *get_or_create_owner(uint64_t owner_id) {
    uint32_t index;
    ResourceOwner *owner = find_owner(owner_id);
    if (owner != 0) return owner;
    for (index = 0; index < BAURA_RESOURCE_OWNER_CAPACITY; ++index) if (!owners[index].active) {
        owners[index].owner_id = owner_id;
        owners[index].usage.limit = 0;
        owners[index].usage.used = 0;
        owners[index].active = 1;
        ++owner_count;
        return &owners[index];
    }
    return 0;
}

void resource_manager_init(void) {
    uint32_t index;
    for (index = 0; index < BAURA_RESOURCE_OWNER_CAPACITY; ++index) owners[index].active = 0;
    owner_count = 0;
}

BauraStatus baura_resource_set_limit(uint64_t owner_id, uint64_t limit) {
    ResourceOwner *owner;
    if (owner_id == 0) return BAURA_ERR_INVALID_ARGUMENT;
    owner = get_or_create_owner(owner_id);
    if (owner == 0) return BAURA_ERR_CAPACITY;
    if (limit < owner->usage.used) return BAURA_ERR_LIMIT;
    owner->usage.limit = limit;
    return BAURA_OK;
}

BauraStatus baura_resource_reserve(uint64_t owner_id, uint64_t amount) {
    ResourceOwner *owner;
    if (owner_id == 0 || amount == 0) return BAURA_ERR_INVALID_ARGUMENT;
    owner = get_or_create_owner(owner_id);
    if (owner == 0) return BAURA_ERR_CAPACITY;
    if (amount > owner->usage.limit - owner->usage.used) return BAURA_ERR_LIMIT;
    owner->usage.used += amount;
    return BAURA_OK;
}

BauraStatus baura_resource_release(uint64_t owner_id, uint64_t amount) {
    ResourceOwner *owner;
    if (owner_id == 0 || amount == 0) return BAURA_ERR_INVALID_ARGUMENT;
    owner = find_owner(owner_id);
    if (owner == 0) return BAURA_ERR_NOT_FOUND;
    if (amount > owner->usage.used) return BAURA_ERR_INVALID_STATE;
    owner->usage.used -= amount;
    return BAURA_OK;
}

BauraStatus baura_resource_query(uint64_t owner_id, BauraResourceUsage *usage) {
    ResourceOwner *owner;
    if (owner_id == 0 || usage == 0) return BAURA_ERR_INVALID_ARGUMENT;
    owner = find_owner(owner_id);
    if (owner == 0) return BAURA_ERR_NOT_FOUND;
    *usage = owner->usage;
    return BAURA_OK;
}

uint32_t baura_resource_owner_count(void) { return owner_count; }
