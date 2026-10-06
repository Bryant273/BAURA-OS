#ifndef BAURA_CORE_ENTRY_H
#define BAURA_CORE_ENTRY_H

#include <stddef.h>

#include "../baura_core_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef BauraStatus (*BauraConsoleWrite)(void *context, const char *text, size_t length);

typedef struct {
    BauraConsoleWrite console_write;
    void *console_context;
} BauraCorePlatform;

BauraStatus baura_core_initial_task_start(const BauraCorePlatform *platform);

#ifdef __cplusplus
}
#endif

#endif
