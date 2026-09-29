#include "baura_event_bus.h"

typedef struct {
    uint32_t token;
    uint32_t topic;
    BauraEventCallback callback;
    void *context;
    uint8_t active;
} BauraSubscription;

static BauraSubscription subscriptions[BAURA_EVENT_SUBSCRIBER_CAPACITY];
static uint32_t subscriber_count;
static uint32_t next_token = 1;

void event_bus_init(void) {
    uint32_t index;
    for (index = 0; index < BAURA_EVENT_SUBSCRIBER_CAPACITY; ++index) {
        subscriptions[index].active = 0;
    }
    subscriber_count = 0;
    next_token = 1;
}

BauraStatus baura_event_subscribe(uint32_t topic, BauraEventCallback callback, void *context, uint32_t *token) {
    uint32_t index;
    if (topic == 0 || callback == 0 || token == 0) return BAURA_ERR_INVALID_ARGUMENT;
    for (index = 0; index < BAURA_EVENT_SUBSCRIBER_CAPACITY; ++index) {
        if (!subscriptions[index].active) {
            subscriptions[index].token = next_token++;
            if (next_token == 0) next_token = 1;
            subscriptions[index].topic = topic;
            subscriptions[index].callback = callback;
            subscriptions[index].context = context;
            subscriptions[index].active = 1;
            ++subscriber_count;
            *token = subscriptions[index].token;
            return BAURA_OK;
        }
    }
    return BAURA_ERR_CAPACITY;
}

BauraStatus baura_event_unsubscribe(uint32_t token) {
    uint32_t index;
    if (token == 0) return BAURA_ERR_INVALID_ARGUMENT;
    for (index = 0; index < BAURA_EVENT_SUBSCRIBER_CAPACITY; ++index) {
        if (subscriptions[index].active && subscriptions[index].token == token) {
            subscriptions[index].active = 0;
            --subscriber_count;
            return BAURA_OK;
        }
    }
    return BAURA_ERR_NOT_FOUND;
}

BauraStatus baura_event_publish(const BauraEvent *event) {
    BauraEventCallback callbacks[BAURA_EVENT_SUBSCRIBER_CAPACITY];
    void *contexts[BAURA_EVENT_SUBSCRIBER_CAPACITY];
    uint32_t count = 0;
    uint32_t index;
    if (event == 0 || event->topic == 0 || event->payload_count > BAURA_EVENT_PAYLOAD_WORDS) return BAURA_ERR_INVALID_ARGUMENT;
    for (index = 0; index < BAURA_EVENT_SUBSCRIBER_CAPACITY; ++index) {
        if (subscriptions[index].active && subscriptions[index].topic == event->topic) {
            callbacks[count] = subscriptions[index].callback;
            contexts[count] = subscriptions[index].context;
            ++count;
        }
    }
    for (index = 0; index < count; ++index) callbacks[index](event, contexts[index]);
    return BAURA_OK;
}

uint32_t baura_event_subscriber_count(void) {
    return subscriber_count;
}
