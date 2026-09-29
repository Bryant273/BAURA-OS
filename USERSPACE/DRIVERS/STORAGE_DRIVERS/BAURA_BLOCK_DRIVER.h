
#ifndef BAURA_BLOCK_DRIVER_H
#define BAURA_BLOCK_DRIVER_H

#include <stdint.h>

typedef struct {
    uintptr_t mmio_base;
    uint32_t irq;
} DriverConfig;

// Driver Entry Points (Implemented in Rust)
extern void* driver_init(uintptr_t mmio, uint32_t irq);
extern int driver_read(void* driver_handle, uint64_t block, uint8_t* buffer);
extern int driver_write(void* driver_handle, uint64_t block, const uint8_t* buffer);

#endif
