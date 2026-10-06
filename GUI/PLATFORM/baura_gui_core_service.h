#ifndef BAURA_GUI_CORE_SERVICE_H
#define BAURA_GUI_CORE_SERVICE_H

#include "../../KERNEL/BAURA_CORE/baura_core_types.h"
#include <stdint.h>

#ifdef __cplusplus
class BauraGuiPlatform;
extern "C" {
#else
typedef struct BauraGuiPlatform BauraGuiPlatform;
#endif

typedef struct {
    BauraGuiPlatform *runtime;
    uint32_t service_id;
} BauraGuiServiceBinding;

BauraStatus baura_gui_core_service_start(void *context);
BauraStatus baura_gui_core_service_stop(BauraGuiServiceBinding *binding);

#ifdef __cplusplus
}
#endif

#endif
