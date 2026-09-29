#ifndef BAURA_CORE_TYPES_H
#define BAURA_CORE_TYPES_H

#include <stdint.h>

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
