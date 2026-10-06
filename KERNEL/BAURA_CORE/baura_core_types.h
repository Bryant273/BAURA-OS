#ifndef BAURA_CORE_TYPES_H
#define BAURA_CORE_TYPES_H

#include <stdint.h>

#if defined(BAURA_TARGET_X86_64) && UINTPTR_MAX != UINT64_MAX
#error "BAURA_TARGET_X86_64 requires a 64-bit target"
#endif

typedef enum {
    BAURA_OK = 0,
    BAURA_ERR_INVALID_ARGUMENT = -1,
    BAURA_ERR_CAPACITY = -2,
    BAURA_ERR_NOT_FOUND = -3,
    BAURA_ERR_ALREADY_EXISTS = -4,
    BAURA_ERR_INVALID_STATE = -5,
    BAURA_ERR_LIMIT = -6
} BauraStatus;

#define BAURA_NAME_MAX 32u

#endif
