#ifndef BAURA_LOGGING_H
#define BAURA_LOGGING_H

#include "../baura_core_types.h"

#define BAURA_LOG_CAPACITY 64u
#define BAURA_LOG_MESSAGE_MAX 96u

typedef enum {
    BAURA_LOG_DEBUG = 0,
    BAURA_LOG_INFO = 1,
    BAURA_LOG_WARNING = 2,
    BAURA_LOG_ERROR = 3
} BauraLogLevel;

typedef struct {
    uint64_t sequence;
    BauraLogLevel level;
    char message[BAURA_LOG_MESSAGE_MAX];
} BauraLogRecord;

void logging_init(void);
BauraStatus baura_log_write(BauraLogLevel level, const char *message);
BauraStatus baura_log_read(uint32_t chronological_index, BauraLogRecord *record);
uint32_t baura_log_count(void);

#endif
