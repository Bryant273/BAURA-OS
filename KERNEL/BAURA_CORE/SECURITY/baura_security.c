#include "baura_security.h"

#define BAURA_CSPACE_CAPACITY 64u
#define BAURA_CAPABILITY_SLOTS 64u
#define BAURA_VALID_RIGHTS (CAP_READ | CAP_WRITE | CAP_EXEC | CAP_GRANT)

typedef struct { uint32_t slot; BauraCapability capability; uint8_t active; } CapabilitySlot;
typedef struct { uint64_t task_id; CapabilitySlot slots[BAURA_CAPABILITY_SLOTS]; uint8_t active; } CapabilitySpace;
static CapabilitySpace spaces[BAURA_CSPACE_CAPACITY];

static int32_t find_space(uint64_t task_id) {
    uint32_t index;
    for (index = 0; index < BAURA_CSPACE_CAPACITY; ++index) if (spaces[index].active && spaces[index].task_id == task_id) return (int32_t)index;
    return -1;
}

void security_init(void) {
    uint32_t index;
    for (index = 0; index < BAURA_CSPACE_CAPACITY; ++index) spaces[index].active = 0;
}

int sec_create_cspace(uint64_t task_id) {
    uint32_t index;
    if (task_id == 0) return -5;
    if (find_space(task_id) >= 0) return -7;
    for (index = 0; index < BAURA_CSPACE_CAPACITY; ++index) if (!spaces[index].active) {
        uint32_t slot;
        spaces[index].task_id = task_id;
        for (slot = 0; slot < BAURA_CAPABILITY_SLOTS; ++slot) spaces[index].slots[slot].active = 0;
        spaces[index].active = 1; return 0;
    }
    return -4;
}

int sec_destroy_cspace(uint64_t task_id) {
    int32_t index = find_space(task_id);
    if (index < 0) return -1;
    spaces[index].active = 0; return 0;
}

int sec_mint_capability(uint64_t task_id, uint32_t slot, uint64_t object_id, uint32_t rights) {
    int32_t space_index = find_space(task_id);
    uint32_t index;
    if (object_id == 0 || rights == 0 || (rights & ~BAURA_VALID_RIGHTS) != 0) return -5;
    if (space_index < 0) return -1;
    for (index = 0; index < BAURA_CAPABILITY_SLOTS; ++index) if (spaces[space_index].slots[index].active && spaces[space_index].slots[index].slot == slot) {
        spaces[space_index].slots[index].capability.object_id = object_id; spaces[space_index].slots[index].capability.rights = rights; return 0;
    }
    for (index = 0; index < BAURA_CAPABILITY_SLOTS; ++index) if (!spaces[space_index].slots[index].active) {
        spaces[space_index].slots[index].slot = slot; spaces[space_index].slots[index].capability.object_id = object_id;
        spaces[space_index].slots[index].capability.rights = rights; spaces[space_index].slots[index].active = 1; return 0;
    }
    return -4;
}

int sec_validate_capability(uint64_t task_id, uint32_t slot, uint32_t required_right) {
    int32_t space_index = find_space(task_id);
    uint32_t index;
    if (required_right == 0 || (required_right & ~BAURA_VALID_RIGHTS) != 0 || space_index < 0) return 0;
    for (index = 0; index < BAURA_CAPABILITY_SLOTS; ++index) if (spaces[space_index].slots[index].active && spaces[space_index].slots[index].slot == slot)
        return (spaces[space_index].slots[index].capability.rights & required_right) == required_right;
    return 0;
}

int sec_revoke_capability(uint64_t task_id, uint32_t slot) {
    int32_t space_index = find_space(task_id);
    uint32_t index;
    if (space_index < 0) return -1;
    for (index = 0; index < BAURA_CAPABILITY_SLOTS; ++index) if (spaces[space_index].slots[index].active && spaces[space_index].slots[index].slot == slot) {
        spaces[space_index].slots[index].active = 0; return 0;
    }
    return -2;
}

int sec_delegate_capability(uint64_t src_task, uint32_t src_slot, uint64_t dst_task, uint32_t dst_slot, uint32_t requested_rights) {
    int32_t source_index = find_space(src_task);
    int32_t destination_index = find_space(dst_task);
    uint32_t source_slot;
    uint32_t destination_slot;
    BauraCapability source;
    if (requested_rights == 0 || (requested_rights & ~BAURA_VALID_RIGHTS) != 0) return -5;
    if (source_index < 0 || destination_index < 0) return -1;
    for (source_slot = 0; source_slot < BAURA_CAPABILITY_SLOTS; ++source_slot) if (spaces[source_index].slots[source_slot].active && spaces[source_index].slots[source_slot].slot == src_slot) break;
    if (source_slot == BAURA_CAPABILITY_SLOTS) return -2;
    source = spaces[source_index].slots[source_slot].capability;
    if ((source.rights & CAP_GRANT) == 0) return -3;
    if ((requested_rights & ~source.rights) != 0) return -6;
    for (destination_slot = 0; destination_slot < BAURA_CAPABILITY_SLOTS; ++destination_slot) if (spaces[destination_index].slots[destination_slot].active && spaces[destination_index].slots[destination_slot].slot == dst_slot) return -7;
    for (destination_slot = 0; destination_slot < BAURA_CAPABILITY_SLOTS; ++destination_slot) if (!spaces[destination_index].slots[destination_slot].active) {
        spaces[destination_index].slots[destination_slot].slot = dst_slot;
        spaces[destination_index].slots[destination_slot].capability.object_id = source.object_id;
        spaces[destination_index].slots[destination_slot].capability.rights = requested_rights;
        spaces[destination_index].slots[destination_slot].active = 1; return 0;
    }
    return -4;
}
