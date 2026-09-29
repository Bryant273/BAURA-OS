#ifndef BAURA_VOLUME_MANAGER_H
#define BAURA_VOLUME_MANAGER_H

#include "../baura_storage.h"

#define BAURA_VOLUME_CAPACITY 16u
#define BAURA_VOLUME_NAME_MAX 8u

void volume_manager_init(void);
BauraStatus baura_volume_register(const char *name, uint32_t volume_id, const BauraBlockDevice *device);
BauraStatus baura_volume_get_id(const char *name, uint32_t *volume_id);
BauraStatus baura_volume_read(uint32_t volume_id, uint64_t lba, uint32_t block_count, void *buffer);
BauraStatus baura_volume_write(uint32_t volume_id, uint64_t lba, uint32_t block_count, const void *buffer);
uint32_t baura_volume_count(void);

#endif
