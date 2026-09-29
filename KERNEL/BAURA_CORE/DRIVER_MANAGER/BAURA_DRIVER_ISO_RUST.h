
#ifndef BAURA_DRIVER_ISO_RUST_H
#define BAURA_DRIVER_ISO_RUST_H

#include <stdint.h>

// Spawns a driver in a dedicated L4 isolated address space
extern int iso_spawn_driver(uint32_t driver_id, const char* driver_type);

// Watchdog: Check all drivers and restart any that have crashed
extern int iso_monitor_drivers();

#endif
