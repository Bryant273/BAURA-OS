#include "volume_manager.h"

typedef struct { char name[BAURA_VOLUME_NAME_MAX]; uint32_t volume_id; BauraBlockDevice device; uint8_t active; } VolumeEntry;
static VolumeEntry volumes[BAURA_VOLUME_CAPACITY];
static uint32_t volume_count;

static int equal(const char *left, const char *right) {
    uint32_t index = 0;
    if (left == 0 || right == 0) return 0;
    while (left[index] != '\0' && left[index] == right[index]) ++index;
    return left[index] == right[index];
}

static VolumeEntry *find_id(uint32_t volume_id) {
    uint32_t index;
    for (index = 0; index < BAURA_VOLUME_CAPACITY; ++index) if (volumes[index].active && volumes[index].volume_id == volume_id) return &volumes[index];
    return 0;
}

void volume_manager_init(void) {
    uint32_t index;
    for (index = 0; index < BAURA_VOLUME_CAPACITY; ++index) volumes[index].active = 0;
    volume_count = 0;
}

BauraStatus baura_volume_register(const char *name, uint32_t volume_id, const BauraBlockDevice *device) {
    uint32_t index;
    uint32_t length = 0;
    if (name == 0 || device == 0 || volume_id == 0 || name[0] == '\0' || device->block_count == 0 || device->block_size == 0 || device->read == 0) return BAURA_ERR_INVALID_ARGUMENT;
    while (length < BAURA_VOLUME_NAME_MAX && name[length] != '\0') {
        char ch = name[length];
        if (!((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_')) return BAURA_ERR_INVALID_ARGUMENT;
        ++length;
    }
    if (length == 0 || length >= BAURA_VOLUME_NAME_MAX) return BAURA_ERR_LIMIT;
    for (index = 0; index < BAURA_VOLUME_CAPACITY; ++index) if (volumes[index].active) {
        if (volumes[index].volume_id == volume_id || equal(volumes[index].name, name)) return BAURA_ERR_ALREADY_EXISTS;
    }
    for (index = 0; index < BAURA_VOLUME_CAPACITY; ++index) if (!volumes[index].active) {
        uint32_t char_index;
        for (char_index = 0; char_index <= length; ++char_index) volumes[index].name[char_index] = name[char_index];
        volumes[index].volume_id = volume_id; volumes[index].device = *device; volumes[index].active = 1; ++volume_count;
        return BAURA_OK;
    }
    return BAURA_ERR_CAPACITY;
}

BauraStatus baura_volume_get_id(const char *name, uint32_t *volume_id) {
    uint32_t index;
    if (name == 0 || volume_id == 0) return BAURA_ERR_INVALID_ARGUMENT;
    for (index = 0; index < BAURA_VOLUME_CAPACITY; ++index) if (volumes[index].active && equal(volumes[index].name, name)) { *volume_id = volumes[index].volume_id; return BAURA_OK; }
    return BAURA_ERR_NOT_FOUND;
}

BauraStatus baura_volume_read(uint32_t volume_id, uint64_t lba, uint32_t block_count, void *buffer) {
    VolumeEntry *volume = find_id(volume_id);
    if (buffer == 0 || block_count == 0) return BAURA_ERR_INVALID_ARGUMENT;
    if (volume == 0) return BAURA_ERR_NOT_FOUND;
    if (lba >= volume->device.block_count || block_count > volume->device.block_count - lba) return BAURA_ERR_LIMIT;
    return volume->device.read(volume->device.context, lba, block_count, buffer);
}

BauraStatus baura_volume_write(uint32_t volume_id, uint64_t lba, uint32_t block_count, const void *buffer) {
    VolumeEntry *volume = find_id(volume_id);
    if (buffer == 0 || block_count == 0) return BAURA_ERR_INVALID_ARGUMENT;
    if (volume == 0) return BAURA_ERR_NOT_FOUND;
    if (volume->device.write == 0) return BAURA_ERR_INVALID_STATE;
    if (lba >= volume->device.block_count || block_count > volume->device.block_count - lba) return BAURA_ERR_LIMIT;
    return volume->device.write(volume->device.context, lba, block_count, buffer);
}

uint32_t baura_volume_count(void) { return volume_count; }
