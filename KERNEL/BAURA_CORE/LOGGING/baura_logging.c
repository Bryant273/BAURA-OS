#include "baura_logging.h"

static BauraLogRecord records[BAURA_LOG_CAPACITY];
static uint64_t next_sequence = 1;
static uint32_t record_count;

void logging_init(void) {
    uint32_t index;
    for (index = 0; index < BAURA_LOG_CAPACITY; ++index) {
        records[index].sequence = 0;
        records[index].message[0] = '\0';
    }
    next_sequence = 1;
    record_count = 0;
}

BauraStatus baura_log_write(BauraLogLevel level, const char *message) {
    uint32_t index;
    BauraLogRecord *record;
    if (message == 0 || (uint32_t)level > (uint32_t)BAURA_LOG_ERROR) {
        return BAURA_ERR_INVALID_ARGUMENT;
    }
    index = (uint32_t)((next_sequence - 1) % BAURA_LOG_CAPACITY);
    record = &records[index];
    record->sequence = next_sequence++;
    record->level = level;
    for (index = 0; index + 1 < BAURA_LOG_MESSAGE_MAX && message[index] != '\0'; ++index) {
        record->message[index] = message[index];
    }
    record->message[index] = '\0';
    if (record_count < BAURA_LOG_CAPACITY) {
        ++record_count;
    }
    return BAURA_OK;
}

BauraStatus baura_log_read(uint32_t chronological_index, BauraLogRecord *record) {
    uint32_t oldest;
    uint32_t slot;
    if (record == 0) {
        return BAURA_ERR_INVALID_ARGUMENT;
    }
    if (chronological_index >= record_count) {
        return BAURA_ERR_NOT_FOUND;
    }
    oldest = (uint32_t)((next_sequence - 1 - record_count) % BAURA_LOG_CAPACITY);
    slot = (oldest + chronological_index) % BAURA_LOG_CAPACITY;
    *record = records[slot];
    return BAURA_OK;
}

uint32_t baura_log_count(void) {
    return record_count;
}
