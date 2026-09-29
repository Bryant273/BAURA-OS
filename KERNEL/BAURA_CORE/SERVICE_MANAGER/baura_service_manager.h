#ifndef BAURA_SERVICE_MANAGER_H
#define BAURA_SERVICE_MANAGER_H

#include "../baura_core_types.h"

#define BAURA_SERVICE_CAPACITY 32u

typedef enum {
    BAURA_SERVICE_STOPPED = 0,
    BAURA_SERVICE_RUNNING = 1,
    BAURA_SERVICE_FAILED = 2
} BauraServiceState;

typedef BauraStatus (*BauraServiceAction)(void *context);

typedef struct {
    uint32_t id;
    BauraServiceState state;
    char name[BAURA_NAME_MAX];
} BauraServiceInfo;

void service_manager_init(void);
BauraStatus baura_service_register(const char *name, BauraServiceAction start, BauraServiceAction stop, void *context, uint32_t *id);
BauraStatus baura_service_start(uint32_t id);
BauraStatus baura_service_stop(uint32_t id);
BauraStatus baura_service_query(uint32_t id, BauraServiceInfo *info);
uint32_t baura_service_count(void);

#endif
