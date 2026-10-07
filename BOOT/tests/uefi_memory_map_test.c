#include <assert.h>
#include <stdint.h>
#include "../FIRMWARE/UEFI/uefi_mem.h"

typedef struct {
    BauraMemoryRegion regions[2];
    uint32_t count;
    int result;
} FakeMap;

static int read_map(void *context, BauraMemoryRegion *regions,
                    uint32_t capacity, uint32_t *count) {
    FakeMap *map = (FakeMap *)context;
    uint32_t index;
    if (map->result != 0) return map->result;
    assert(capacity >= map->count);
    for (index = 0; index < map->count; ++index) {
        regions[index] = map->regions[index];
    }
    *count = map->count;
    return 0;
}

int main(void) {
    FakeMap fake = {
        {
            {0x1000, 0x1000, 0, BAURA_MEMORY_USABLE},
            {0x3000, 0x1000, 0, BAURA_MEMORY_RESERVED}
        },
        2,
        0
    };
    BauraFirmwareMemoryMapProvider provider = {read_map, &fake};
    BauraMemoryRegion result[2];
    uint32_t count = 99;

    assert(baura_firmware_get_memory_map(&provider, result, 2, &count) ==
           BAURA_MEMORY_MAP_OK);
    assert(count == 2 && result[1].physical_start == 0x3000);
    assert(baura_firmware_get_memory_map(&provider, result, 1, &count) ==
           BAURA_MEMORY_MAP_ERR_CAPACITY);
    assert(count == 0);
    fake.regions[1].physical_start = 0x1800;
    assert(baura_firmware_get_memory_map(&provider, result, 2, &count) ==
           BAURA_MEMORY_MAP_ERR_OVERLAP);
    fake.regions[1].physical_start = UINT64_MAX - 1;
    fake.regions[1].length = 4;
    assert(baura_firmware_get_memory_map(&provider, result, 2, &count) ==
           BAURA_MEMORY_MAP_ERR_INVALID_REGION);
    fake.result = -1;
    assert(baura_firmware_get_memory_map(&provider, result, 2, &count) ==
           BAURA_MEMORY_MAP_ERR_PROVIDER);
    return 0;
}
