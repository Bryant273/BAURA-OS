#include "baura_storage.h"
#include "VOLUME_MANAGER/volume_manager.h"

static uint8_t storage_ready;

void storage_init(void) { storage_ready = 1; }

BauraStatus baura_storage_read(uint32_t volume_id, uint64_t lba, uint32_t block_count, void *buffer) {
    if (!storage_ready) return BAURA_ERR_INVALID_STATE;
    return baura_volume_read(volume_id, lba, block_count, buffer);
}

BauraStatus baura_storage_write(uint32_t volume_id, uint64_t lba, uint32_t block_count, const void *buffer) {
    if (!storage_ready) return BAURA_ERR_INVALID_STATE;
    return baura_volume_write(volume_id, lba, block_count, buffer);
}
