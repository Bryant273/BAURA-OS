#ifndef BAURA_STORAGE_H
#define BAURA_STORAGE_H

#include "../baura_core_types.h"

typedef BauraStatus (*BauraBlockRead)(void *context, uint64_t lba, uint32_t block_count, void *buffer);
typedef BauraStatus (*BauraBlockWrite)(void *context, uint64_t lba, uint32_t block_count, const void *buffer);

typedef struct {
    void *context;
    uint64_t block_count;
    uint32_t block_size;
    BauraBlockRead read;
    BauraBlockWrite write;
} BauraBlockDevice;

void storage_init(void);
BauraStatus baura_storage_read(uint32_t volume_id, uint64_t lba, uint32_t block_count, void *buffer);
BauraStatus baura_storage_write(uint32_t volume_id, uint64_t lba, uint32_t block_count, const void *buffer);

#endif
