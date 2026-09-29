
#ifndef BAURA_MESSAGE_BUS_GO_H
#define BAURA_MESSAGE_BUS_GO_H

#include <stdint.h>
#include <stddef.h>

// Publish a message to a specific topic
extern void baura_bus_publish(uint32_t topic, const char* data, int size);

// Subscribe to a topic
extern void baura_bus_subscribe(uint32_t topic, void* callback);

#endif
