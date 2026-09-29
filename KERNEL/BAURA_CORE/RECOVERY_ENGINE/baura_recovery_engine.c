#include "baura_recovery_engine.h"

typedef struct { uint32_t fault_code; BauraRecoveryHandler handler; void *context; uint8_t active; } RecoveryPolicy;
static RecoveryPolicy policies[BAURA_RECOVERY_POLICY_CAPACITY];

void recovery_engine_init(void) {
    uint32_t index;
    for (index = 0; index < BAURA_RECOVERY_POLICY_CAPACITY; ++index) policies[index].active = 0;
}

BauraStatus baura_recovery_register(uint32_t fault_code, BauraRecoveryHandler handler, void *context) {
    uint32_t index;
    if (fault_code == 0 || handler == 0) return BAURA_ERR_INVALID_ARGUMENT;
    for (index = 0; index < BAURA_RECOVERY_POLICY_CAPACITY; ++index) {
        if (policies[index].active && policies[index].fault_code == fault_code) {
            policies[index].handler = handler; policies[index].context = context; return BAURA_OK;
        }
    }
    for (index = 0; index < BAURA_RECOVERY_POLICY_CAPACITY; ++index) if (!policies[index].active) {
        policies[index].fault_code = fault_code; policies[index].handler = handler;
        policies[index].context = context; policies[index].active = 1; return BAURA_OK;
    }
    return BAURA_ERR_CAPACITY;
}

BauraStatus baura_recovery_execute(const BauraFaultRecord *fault) {
    uint32_t index;
    if (fault == 0 || fault->code == 0) return BAURA_ERR_INVALID_ARGUMENT;
    for (index = 0; index < BAURA_RECOVERY_POLICY_CAPACITY; ++index)
        if (policies[index].active && policies[index].fault_code == fault->code) return policies[index].handler(fault, policies[index].context);
    return BAURA_ERR_NOT_FOUND;
}
