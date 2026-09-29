#include "baura_monitoring.h"

static BauraMetricSample samples[BAURA_METRIC_SAMPLE_CAPACITY];
static uint64_t next_sequence = 1;
static uint32_t sample_count;

void monitoring_init(void) { next_sequence = 1; sample_count = 0; }

BauraStatus baura_monitor_record(uint32_t metric_id, uint64_t timestamp, int64_t value) {
    uint32_t slot;
    if (metric_id == 0) return BAURA_ERR_INVALID_ARGUMENT;
    slot = (uint32_t)((next_sequence - 1) % BAURA_METRIC_SAMPLE_CAPACITY);
    samples[slot].metric_id = metric_id; samples[slot].timestamp = timestamp;
    samples[slot].value = value; samples[slot].sequence = next_sequence++;
    if (sample_count < BAURA_METRIC_SAMPLE_CAPACITY) ++sample_count;
    return BAURA_OK;
}

BauraStatus baura_monitor_read_latest(uint32_t metric_id, uint32_t age, BauraMetricSample *sample) {
    uint32_t offset;
    uint32_t found = 0;
    if (metric_id == 0 || sample == 0) return BAURA_ERR_INVALID_ARGUMENT;
    for (offset = 0; offset < sample_count; ++offset) {
        uint32_t slot = (uint32_t)((next_sequence - 2 - offset) % BAURA_METRIC_SAMPLE_CAPACITY);
        if (samples[slot].metric_id == metric_id) {
            if (found == age) { *sample = samples[slot]; return BAURA_OK; }
            ++found;
        }
    }
    return BAURA_ERR_NOT_FOUND;
}

uint32_t baura_monitor_count(void) { return sample_count; }
