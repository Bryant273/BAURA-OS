#ifndef BAURA_MESSAGE_BUS_H
#define BAURA_MESSAGE_BUS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BAURA_BUS_MAX_SUBSCRIBERS 64u

typedef struct {
    uint32_t topic_id;
    uint32_t priority;
    const void *data;
    uint32_t size;
} BusMessage;

typedef void (*BauraBusCallback)(const BusMessage *message, void *context);

enum {
    BAURA_BUS_OK = 0,
    BAURA_BUS_ERR_ARGUMENT = -1,
    BAURA_BUS_ERR_CAPACITY = -2,
    BAURA_BUS_ERR_NOT_FOUND = -3
};

int bus_publish(uint32_t topic, const void *data, uint32_t size);
int bus_subscribe(uint32_t topic, BauraBusCallback callback, void *context,
                  uint32_t *subscription_id);
int bus_unsubscribe(uint32_t subscription_id);

#ifdef __cplusplus
}
#endif

#endif
