#ifndef BAURA_EVENT_BUS_H
#define BAURA_EVENT_BUS_H

#include "../baura_core_types.h"

#define BAURA_EVENT_SUBSCRIBER_CAPACITY 32u
#define BAURA_EVENT_PAYLOAD_WORDS 4u

typedef struct {
    uint32_t topic;
    uint32_t source;
    uint32_t payload_count;
    uintptr_t payload[BAURA_EVENT_PAYLOAD_WORDS];
} BauraEvent;

typedef void (*BauraEventCallback)(const BauraEvent *event, void *context);

void event_bus_init(void);
BauraStatus baura_event_subscribe(uint32_t topic, BauraEventCallback callback, void *context, uint32_t *token);
BauraStatus baura_event_unsubscribe(uint32_t token);
BauraStatus baura_event_publish(const BauraEvent *event);
uint32_t baura_event_subscriber_count(void);

#endif
