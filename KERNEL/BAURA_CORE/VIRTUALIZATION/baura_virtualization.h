#ifndef BAURA_VIRTUALIZATION_H
#define BAURA_VIRTUALIZATION_H

#include "../baura_core_types.h"

#define BAURA_VM_CAPACITY 16u
typedef struct { uint64_t memory_bytes; uint32_t vcpu_count; } BauraVmConfig;
typedef BauraStatus (*BauraVmCapabilityProbe)(void *context, uint8_t *available);
typedef BauraStatus (*BauraVmCreate)(const BauraVmConfig *config, void *context, uint64_t *vm_id);
typedef BauraStatus (*BauraVmDestroy)(uint64_t vm_id, void *context);

void virtualization_init(void);
BauraStatus baura_virtualization_set_provider(BauraVmCapabilityProbe probe, BauraVmCreate create, BauraVmDestroy destroy, void *context);
BauraStatus baura_virtualization_available(uint8_t *available);
BauraStatus baura_vm_create(const BauraVmConfig *config, uint64_t *vm_id);
BauraStatus baura_vm_destroy(uint64_t vm_id);
uint32_t baura_vm_count(void);

#endif
