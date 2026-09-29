#ifndef BAURA_HARDWARE_MANAGER_H
#define BAURA_HARDWARE_MANAGER_H

#include "../DEVICE_MANAGER/BAURA_DEVICE_MGR.h"

typedef struct { uint32_t total; uint32_t by_type[5]; } BauraHardwareSummary;

void hardware_manager_init(void);
BauraStatus baura_hardware_refresh(void);
BauraStatus baura_hardware_get_summary(BauraHardwareSummary *summary);

#endif
