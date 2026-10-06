#ifndef BAURA_CORE_ENTRY_H
#define BAURA_CORE_ENTRY_H

#include <stddef.h>

#include "../baura_core_types.h"
#include "../PROCESS_MANAGER/baura_process_manager.h"
#include "../SECURITY/baura_security.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef BauraStatus (*BauraConsoleWrite)(void *context, const char *text, size_t length);
typedef BauraStatus (*BauraBootInfoValidate)(void *context, const void *boot_info);

typedef BauraStatus (*BauraStartGraphicalService)(void *context);

#define BAURA_CORE_PLATFORM_ABI_VERSION 1u

typedef struct {
    uint32_t abi_version;
    uint32_t struct_size;
    const void *boot_info;
    BauraBootInfoValidate validate_boot_info;
    void *boot_info_context;
    BauraConsoleWrite console_write;
    void *console_context;
    BauraProcessBackend process_backend;
    BauraSecurityAuthorityCheck authority_check;
    void *authority_context;
    BauraStartGraphicalService start_graphical_service;
    void *graphical_service_context;
} BauraCorePlatform;

BauraStatus baura_core_initial_task_start(const BauraCorePlatform *platform);

#ifdef __cplusplus
}
#endif

#endif
