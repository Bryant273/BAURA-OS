#ifndef BAURA_MONITORING_H
#define BAURA_MONITORING_H

#include "../baura_core_types.h"

#define BAURA_METRIC_SAMPLE_CAPACITY 128u

typedef struct { uint32_t metric_id; uint64_t timestamp; int64_t value; uint64_t sequence; } BauraMetricSample;

void monitoring_init(void);
BauraStatus baura_monitor_record(uint32_t metric_id, uint64_t timestamp, int64_t value);
BauraStatus baura_monitor_read_latest(uint32_t metric_id, uint32_t age, BauraMetricSample *sample);
uint32_t baura_monitor_count(void);

#endif
