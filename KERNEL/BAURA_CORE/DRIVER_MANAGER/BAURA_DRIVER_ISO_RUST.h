
#ifndef BAURA_DRIVER_ISO_RUST_H
#define BAURA_DRIVER_ISO_RUST_H

#include <stddef.h>
#include <stdint.h>

enum {
    BAURA_DRIVER_ISO_ABI_VERSION = 1u,
    BAURA_DRIVER_ISO_OK = 0,
    BAURA_DRIVER_ISO_ERR_ARGUMENT = -1,
    BAURA_DRIVER_ISO_ERR_UNAVAILABLE = -2,
    BAURA_DRIVER_ISO_ERR_CAPACITY = -3,
    BAURA_DRIVER_ISO_ERR_NOT_FOUND = -4,
    BAURA_DRIVER_ISO_ERR_BACKEND = -5,
    BAURA_DRIVER_ISO_ERR_ALREADY_EXISTS = -6,
    BAURA_DRIVER_ISO_ERR_LIMIT = -7,
    BAURA_DRIVER_ISO_ERR_INVALID_STATE = -8
};

typedef int (*BauraDriverSpawn)(
    void *context, uint32_t driver_id, const char *driver_type,
    uint64_t *task_id, uint64_t *address_space_id);
typedef int (*BauraDriverTaskAlive)(
    void *context, uint64_t task_id, uint8_t *alive);
typedef int (*BauraDriverTerminate)(
    void *context, uint64_t task_id, uint64_t address_space_id);

/* The backend must create/destroy real platform objects or return an error. */
typedef struct {
    uint32_t abi_version;
    uint32_t struct_size;
    void *context;
    BauraDriverSpawn spawn_driver;
    BauraDriverTaskAlive is_task_alive;
    BauraDriverTerminate terminate_task;
} BauraDriverIsolationBackend;

#ifdef __cplusplus
extern "C" {
#endif

int iso_set_platform_backend(const BauraDriverIsolationBackend *backend);
int iso_spawn_driver(uint32_t driver_id, const char *driver_type);

int iso_monitor_drivers(void);
int iso_stop_driver(uint32_t driver_id);

#ifdef __cplusplus
}
#endif

#endif
