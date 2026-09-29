#ifndef BAURA_FAULT_ENGINE_H
#define BAURA_FAULT_ENGINE_H

#include "../baura_core_types.h"

#define BAURA_FAULT_CAPACITY 32u
#define BAURA_EVENT_SYSTEM_FAULT 0xFFFF0001u

typedef enum { BAURA_FAULT_INFO, BAURA_FAULT_WARNING, BAURA_FAULT_RECOVERABLE, BAURA_FAULT_FATAL } BauraFaultSeverity;
typedef struct { uint64_t id; uint64_t timestamp; uint32_t code; uint32_t source; BauraFaultSeverity severity; } BauraFaultRecord;

void fault_engine_init(void);
BauraStatus baura_fault_report(uint32_t code, uint32_t source, uint64_t timestamp, BauraFaultSeverity severity, uint64_t *fault_id);
BauraStatus baura_fault_read(uint32_t chronological_index, BauraFaultRecord *record);
uint32_t baura_fault_count(void);

#endif
