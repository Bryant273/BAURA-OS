
#ifndef BAURA_MESSAGE_BUS_GO_H
#define BAURA_MESSAGE_BUS_GO_H

#include <stdint.h>
#include <stddef.h>

typedef void (*BauraGoBusCallback)(uint32_t topic, const char *data, int size);

#define BAURA_GO_BUS_MAX_SUBSCRIBERS 64

enum {
    BAURA_GO_BUS_OK = 0,
    BAURA_GO_BUS_ERR_ARGUMENT = -1,
    BAURA_GO_BUS_ERR_CAPACITY = -2,
    BAURA_GO_BUS_ERR_NOT_FOUND = -3
};

extern int baura_bus_publish(uint32_t topic, const char* data, int size);
extern int baura_bus_subscribe(uint32_t topic, void *callback);
extern int baura_bus_unsubscribe(uint32_t topic, void *callback);

#endif
