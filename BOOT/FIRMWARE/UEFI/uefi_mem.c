#include "uefi_mem.h"

#include <stddef.h>

int baura_firmware_get_memory_map(
    const BauraFirmwareMemoryMapProvider *provider, BauraMemoryRegion *regions,
    uint32_t capacity, uint32_t *region_count) {
    BauraMemoryRegion snapshot[BAURA_MEMORY_MAP_MAX_REGIONS];
    uint32_t count = 0;
    uint32_t index;
    if (provider == 0 || provider->read == 0 || regions == 0 ||
        region_count == 0) {
        return BAURA_MEMORY_MAP_ERR_ARGUMENT;
    }
    *region_count = 0;
    if (capacity == 0) {
        return BAURA_MEMORY_MAP_ERR_CAPACITY;
    }
    if (provider->read(provider->context, snapshot,
                       BAURA_MEMORY_MAP_MAX_REGIONS, &count) != 0) {
        return BAURA_MEMORY_MAP_ERR_PROVIDER;
    }
    if (count > BAURA_MEMORY_MAP_MAX_REGIONS) {
        return BAURA_MEMORY_MAP_ERR_CAPACITY;
    }
    if (count > capacity) return BAURA_MEMORY_MAP_ERR_CAPACITY;

    for (index = 0; index < count; ++index) {
        uint32_t other;
        const BauraMemoryRegion *region = &snapshot[index];
        if (region->length == 0 ||
            region->length > UINT64_MAX - region->physical_start ||
            region->type < BAURA_MEMORY_USABLE ||
            region->type > BAURA_MEMORY_DEVICE) {
            return BAURA_MEMORY_MAP_ERR_INVALID_REGION;
        }
        for (other = 0; other < index; ++other) {
            const BauraMemoryRegion *previous = &snapshot[other];
            const uint64_t region_end =
                region->physical_start + region->length;
            const uint64_t previous_end =
                previous->physical_start + previous->length;
            if (region->physical_start < previous_end &&
                previous->physical_start < region_end) {
                return BAURA_MEMORY_MAP_ERR_OVERLAP;
            }
        }
    }
    for (index = 0; index < count; ++index) regions[index] = snapshot[index];
    *region_count = count;
    return BAURA_MEMORY_MAP_OK;
}
