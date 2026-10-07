#include "BAURA_MESSAGE_BUS.h"

typedef struct {
    uint32_t topic;
    uint32_t id;
    BauraBusCallback callback;
    void *context;
    uint8_t active;
} BusSubscriber;

static BusSubscriber subscribers[BAURA_BUS_MAX_SUBSCRIBERS];
static uint32_t next_subscription_id = 1;

static int subscription_id_in_use(uint32_t id) {
    uint32_t index;
    for (index = 0; index < BAURA_BUS_MAX_SUBSCRIBERS; ++index) {
        if (subscribers[index].active && subscribers[index].id == id) {
            return 1;
        }
    }
    return 0;
}

int bus_subscribe(uint32_t topic, BauraBusCallback callback, void *context,
                  uint32_t *subscription_id) {
    uint32_t index;
    if (topic == 0 || callback == 0 || subscription_id == 0) {
        return BAURA_BUS_ERR_ARGUMENT;
    }
    for (index = 0; index < BAURA_BUS_MAX_SUBSCRIBERS; ++index) {
        if (!subscribers[index].active) {
            uint32_t attempts = 0;
            uint32_t id;
            do {
                id = next_subscription_id++;
                if (next_subscription_id == 0) next_subscription_id = 1;
                if (id == 0) continue;
                ++attempts;
            } while ((id == 0 || subscription_id_in_use(id)) &&
                     attempts <= BAURA_BUS_MAX_SUBSCRIBERS);
            if (id == 0 || subscription_id_in_use(id)) {
                return BAURA_BUS_ERR_CAPACITY;
            }
            subscribers[index].topic = topic;
            subscribers[index].id = id;
            subscribers[index].callback = callback;
            subscribers[index].context = context;
            subscribers[index].active = 1;
            *subscription_id = id;
            return BAURA_BUS_OK;
        }
    }
    return BAURA_BUS_ERR_CAPACITY;
}

int bus_unsubscribe(uint32_t subscription_id) {
    uint32_t index;
    if (subscription_id == 0) return BAURA_BUS_ERR_ARGUMENT;
    for (index = 0; index < BAURA_BUS_MAX_SUBSCRIBERS; ++index) {
        if (subscribers[index].active &&
            subscribers[index].id == subscription_id) {
            subscribers[index].active = 0;
            return BAURA_BUS_OK;
        }
    }
    return BAURA_BUS_ERR_NOT_FOUND;
}

int bus_publish(uint32_t topic, const void *data, uint32_t size) {
    BusSubscriber snapshot[BAURA_BUS_MAX_SUBSCRIBERS];
    BusMessage message;
    uint32_t count = 0;
    uint32_t index;
    if (topic == 0 || (size != 0 && data == 0)) {
        return BAURA_BUS_ERR_ARGUMENT;
    }
    for (index = 0; index < BAURA_BUS_MAX_SUBSCRIBERS; ++index) {
        if (subscribers[index].active && subscribers[index].topic == topic) {
            snapshot[count++] = subscribers[index];
        }
    }
    message.topic_id = topic;
    message.priority = 0;
    message.data = data;
    message.size = size;
    for (index = 0; index < count; ++index) {
        snapshot[index].callback(&message, snapshot[index].context);
    }
    return BAURA_BUS_OK;
}
