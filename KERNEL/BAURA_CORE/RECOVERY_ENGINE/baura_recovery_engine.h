#ifndef BAURA_RECOVERY_ENGINE_H
#define BAURA_RECOVERY_ENGINE_H

#include "../FAULT_ENGINE/baura_fault_engine.h"

#define BAURA_RECOVERY_POLICY_CAPACITY 16u
typedef BauraStatus (*BauraRecoveryHandler)(const BauraFaultRecord *fault, void *context);

void recovery_engine_init(void);
BauraStatus baura_recovery_register(uint32_t fault_code, BauraRecoveryHandler handler, void *context);
BauraStatus baura_recovery_execute(const BauraFaultRecord *fault);

#endif
