#include "baura_service_manager.h"

typedef struct {
    BauraServiceInfo info;
    BauraServiceAction start;
    BauraServiceAction stop;
    void *context;
    uint8_t active;
} ServiceEntry;

static ServiceEntry services[BAURA_SERVICE_CAPACITY];
static uint32_t service_count;
static uint32_t next_id = 1;

static uint32_t copy_name(char *destination, const char *source) {
    uint32_t index;
    if (source == 0 || source[0] == '\0') return 0;
    for (index = 0; index + 1 < BAURA_NAME_MAX && source[index] != '\0'; ++index) destination[index] = source[index];
    if (source[index] != '\0') return 0;
    destination[index] = '\0';
    return 1;
}

static ServiceEntry *find_service(uint32_t id) {
    uint32_t index;
    for (index = 0; index < BAURA_SERVICE_CAPACITY; ++index) if (services[index].active && services[index].info.id == id) return &services[index];
    return 0;
}

void service_manager_init(void) {
    uint32_t index;
    for (index = 0; index < BAURA_SERVICE_CAPACITY; ++index) services[index].active = 0;
    service_count = 0;
    next_id = 1;
}

BauraStatus baura_service_register(const char *name, BauraServiceAction start, BauraServiceAction stop, void *context, uint32_t *id) {
    uint32_t index;
    char checked_name[BAURA_NAME_MAX] = {0};
    if (id == 0 || !copy_name(checked_name, name)) return BAURA_ERR_INVALID_ARGUMENT;
    for (index = 0; index < BAURA_SERVICE_CAPACITY; ++index) {
        uint32_t other;
        if (!services[index].active) continue;
        for (other = 0; checked_name[other] != '\0' && services[index].info.name[other] == checked_name[other]; ++other) {}
        if (checked_name[other] == '\0' && services[index].info.name[other] == '\0') return BAURA_ERR_ALREADY_EXISTS;
    }
    for (index = 0; index < BAURA_SERVICE_CAPACITY; ++index) if (!services[index].active) {
        services[index].info.id = next_id++;
        if (next_id == 0) next_id = 1;
        services[index].info.state = BAURA_SERVICE_STOPPED;
        for (uint32_t char_index = 0; char_index < BAURA_NAME_MAX; ++char_index) services[index].info.name[char_index] = checked_name[char_index];
        services[index].start = start; services[index].stop = stop; services[index].context = context; services[index].active = 1;
        ++service_count; *id = services[index].info.id; return BAURA_OK;
    }
    return BAURA_ERR_CAPACITY;
}

BauraStatus baura_service_start(uint32_t id) {
    ServiceEntry *service = find_service(id);
    BauraStatus status;
    if (service == 0) return BAURA_ERR_NOT_FOUND;
    if (service->info.state == BAURA_SERVICE_RUNNING) return BAURA_OK;
    status = service->start == 0 ? BAURA_OK : service->start(service->context);
    service->info.state = status == BAURA_OK ? BAURA_SERVICE_RUNNING : BAURA_SERVICE_FAILED;
    return status;
}

BauraStatus baura_service_stop(uint32_t id) {
    ServiceEntry *service = find_service(id);
    BauraStatus status;
    if (service == 0) return BAURA_ERR_NOT_FOUND;
    if (service->info.state == BAURA_SERVICE_STOPPED) return BAURA_OK;
    status = service->stop == 0 ? BAURA_OK : service->stop(service->context);
    if (status == BAURA_OK) service->info.state = BAURA_SERVICE_STOPPED;
    return status;
}

BauraStatus baura_service_query(uint32_t id, BauraServiceInfo *info) {
    ServiceEntry *service = find_service(id);
    if (service == 0) return BAURA_ERR_NOT_FOUND;
    if (info == 0) return BAURA_ERR_INVALID_ARGUMENT;
    *info = service->info; return BAURA_OK;
}

uint32_t baura_service_count(void) { return service_count; }
