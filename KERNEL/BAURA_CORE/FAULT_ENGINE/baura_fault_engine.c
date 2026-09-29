#include "baura_fault_engine.h"
#include "../EVENT_BUS/baura_event_bus.h"
#include "../LOGGING/baura_logging.h"

static BauraFaultRecord faults[BAURA_FAULT_CAPACITY];
static uint64_t next_fault_id = 1;
static uint32_t fault_count;

void fault_engine_init(void) { next_fault_id = 1; fault_count = 0; }

BauraStatus baura_fault_report(uint32_t code, uint32_t source, uint64_t timestamp, BauraFaultSeverity severity, uint64_t *fault_id) {
    uint64_t id;
    uint32_t slot;
    BauraEvent event;
    const char *message;
    if (code == 0 || fault_id == 0 || (uint32_t)severity > (uint32_t)BAURA_FAULT_FATAL) return BAURA_ERR_INVALID_ARGUMENT;
    id = next_fault_id++; if (next_fault_id == 0) next_fault_id = 1;
    slot = (uint32_t)((id - 1) % BAURA_FAULT_CAPACITY);
    faults[slot].id = id; faults[slot].timestamp = timestamp;
    faults[slot].code = code; faults[slot].source = source; faults[slot].severity = severity;
    if (fault_count < BAURA_FAULT_CAPACITY) ++fault_count;
    *fault_id = id;
    message = severity == BAURA_FAULT_FATAL ? "Fatal system fault" : "System fault reported";
    baura_log_write(severity >= BAURA_FAULT_RECOVERABLE ? BAURA_LOG_ERROR : BAURA_LOG_WARNING, message);
    event.topic = BAURA_EVENT_SYSTEM_FAULT; event.source = source; event.payload_count = 2;
    event.payload[0] = (uintptr_t)code; event.payload[1] = (uintptr_t)id;
    (void)baura_event_publish(&event);
    return BAURA_OK;
}

BauraStatus baura_fault_read(uint32_t chronological_index, BauraFaultRecord *record) {
    uint32_t oldest;
    if (record == 0) return BAURA_ERR_INVALID_ARGUMENT;
    if (chronological_index >= fault_count) return BAURA_ERR_NOT_FOUND;
    oldest = (uint32_t)((next_fault_id - 1 - fault_count) % BAURA_FAULT_CAPACITY);
    *record = faults[(oldest + chronological_index) % BAURA_FAULT_CAPACITY];
    return BAURA_OK;
}

uint32_t baura_fault_count(void) { return fault_count; }
