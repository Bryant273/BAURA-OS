#ifndef BAURA_UEFI_MEMORY_H
#define BAURA_UEFI_MEMORY_H

#include <stdint.h>

#define BAURA_MEMORY_MAP_MAX_REGIONS 256u

typedef enum {
    BAURA_MEMORY_USABLE = 1,
    BAURA_MEMORY_RESERVED = 2,
    BAURA_MEMORY_FIRMWARE = 3,
    BAURA_MEMORY_RUNTIME = 4,
    BAURA_MEMORY_DEVICE = 5
} BauraMemoryRegionType;

typedef struct {
    uint64_t physical_start;
    uint64_t length;
    uint64_t attributes;
    BauraMemoryRegionType type;
} BauraMemoryRegion;

typedef int (*BauraFirmwareMemoryMapRead)(
    void *context, BauraMemoryRegion *regions, uint32_t capacity,
    uint32_t *region_count);

typedef struct {
    BauraFirmwareMemoryMapRead read;
    void *context;
} BauraFirmwareMemoryMapProvider;

enum {
    BAURA_MEMORY_MAP_OK = 0,
    BAURA_MEMORY_MAP_ERR_ARGUMENT = -1,
    BAURA_MEMORY_MAP_ERR_PROVIDER = -2,
    BAURA_MEMORY_MAP_ERR_CAPACITY = -3,
    BAURA_MEMORY_MAP_ERR_INVALID_REGION = -4,
    BAURA_MEMORY_MAP_ERR_OVERLAP = -5
};

int baura_firmware_get_memory_map(
    const BauraFirmwareMemoryMapProvider *provider, BauraMemoryRegion *regions,
    uint32_t capacity, uint32_t *region_count);

#endif
