#ifndef BAURA_MESSAGE_BUS_H
#define BAURA_MESSAGE_BUS_H

#include <stdint.h>

typedef struct {
    uint32_t topic_id;
    uint32_t priority;
    void* data;
    uint32_t size;
} BusMessage;

void bus_publish(uint32_t topic, void* data, uint32_t size);
void bus_subscribe(uint32_t topic, void (*callback)(BusMessage*));

#endif