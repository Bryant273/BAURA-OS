#include "baura_virtualization.h"

typedef struct { uint64_t id; uint8_t active; } VmEntry;
static VmEntry virtual_machines[BAURA_VM_CAPACITY];
static BauraVmCapabilityProbe capability_probe;
static BauraVmCreate create_vm;
static BauraVmDestroy destroy_vm;
static void *provider_context;
static uint32_t vm_count;

void virtualization_init(void) {
    uint32_t index;
    for (index = 0; index < BAURA_VM_CAPACITY; ++index) virtual_machines[index].active = 0;
    capability_probe = 0; create_vm = 0; destroy_vm = 0; provider_context = 0; vm_count = 0;
}

BauraStatus baura_virtualization_set_provider(BauraVmCapabilityProbe probe, BauraVmCreate create, BauraVmDestroy destroy, void *context) {
    if (probe == 0 || create == 0 || destroy == 0) return BAURA_ERR_INVALID_ARGUMENT;
    if (vm_count != 0) return BAURA_ERR_INVALID_STATE;
    capability_probe = probe; create_vm = create; destroy_vm = destroy; provider_context = context; return BAURA_OK;
}

BauraStatus baura_virtualization_available(uint8_t *available) {
    if (available == 0) return BAURA_ERR_INVALID_ARGUMENT;
    if (capability_probe == 0) return BAURA_ERR_INVALID_STATE;
    return capability_probe(provider_context, available);
}

BauraStatus baura_vm_create(const BauraVmConfig *config, uint64_t *vm_id) {
    uint32_t index;
    uint8_t available = 0;
    BauraStatus status;
    if (config == 0 || vm_id == 0 || config->memory_bytes == 0 || config->vcpu_count == 0) return BAURA_ERR_INVALID_ARGUMENT;
    if (create_vm == 0 || capability_probe == 0) return BAURA_ERR_INVALID_STATE;
    for (index = 0; index < BAURA_VM_CAPACITY; ++index) if (!virtual_machines[index].active) break;
    if (index == BAURA_VM_CAPACITY) return BAURA_ERR_CAPACITY;
    status = capability_probe(provider_context, &available);
    if (status != BAURA_OK) return status;
    if (!available) return BAURA_ERR_INVALID_STATE;
    status = create_vm(config, provider_context, vm_id);
    if (status != BAURA_OK) return status;
    if (*vm_id == 0) return BAURA_ERR_INVALID_STATE;
    for (uint32_t existing = 0; existing < BAURA_VM_CAPACITY; ++existing) {
        if (virtual_machines[existing].active && virtual_machines[existing].id == *vm_id) return BAURA_ERR_ALREADY_EXISTS;
    }
    virtual_machines[index].id = *vm_id; virtual_machines[index].active = 1; ++vm_count;
    return BAURA_OK;
}

BauraStatus baura_vm_destroy(uint64_t vm_id) {
    uint32_t index;
    BauraStatus status;
    if (vm_id == 0) return BAURA_ERR_INVALID_ARGUMENT;
    if (destroy_vm == 0) return BAURA_ERR_INVALID_STATE;
    for (index = 0; index < BAURA_VM_CAPACITY; ++index) if (virtual_machines[index].active && virtual_machines[index].id == vm_id) {
        status = destroy_vm(vm_id, provider_context);
        if (status != BAURA_OK) return status;
        virtual_machines[index].active = 0; --vm_count; return BAURA_OK;
    }
    return BAURA_ERR_NOT_FOUND;
}

uint32_t baura_vm_count(void) { return vm_count; }
