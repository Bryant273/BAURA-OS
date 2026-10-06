#include "baura_gui_core_service.h"

#include "baura_gui_platform.h"
#include "../../KERNEL/BAURA_CORE/SERVICE_MANAGER/baura_service_manager.h"

static BauraStatus start_runtime(void *context) {
    BauraGuiPlatform *runtime = static_cast<BauraGuiPlatform *>(context);
    if (runtime == nullptr) return BAURA_ERR_INVALID_ARGUMENT;
    if (runtime->start()) return BAURA_OK;
    switch (runtime->last_error()) {
    case BAURA_GUI_PLATFORM_ERR_ARGUMENT:
        return BAURA_ERR_INVALID_ARGUMENT;
    case BAURA_GUI_PLATFORM_ERR_STATE:
        return BAURA_ERR_INVALID_STATE;
    default:
        return BAURA_ERR_IO;
    }
}

static BauraStatus stop_runtime(void *context) {
    BauraGuiPlatform *runtime = static_cast<BauraGuiPlatform *>(context);
    if (runtime == nullptr) return BAURA_ERR_INVALID_ARGUMENT;
    runtime->stop();
    return BAURA_OK;
}

extern "C" BauraStatus baura_gui_core_service_start(void *context) {
    BauraGuiServiceBinding *binding =
        static_cast<BauraGuiServiceBinding *>(context);
    if (binding == nullptr || binding->runtime == nullptr) {
        return BAURA_ERR_INVALID_ARGUMENT;
    }
    if (binding->service_id == 0) {
        const BauraStatus registration = baura_service_register(
            "baura.gui", start_runtime, stop_runtime, binding->runtime,
            &binding->service_id);
        if (registration != BAURA_OK) return registration;
    }
    return baura_service_start(binding->service_id);
}

extern "C" BauraStatus baura_gui_core_service_stop(
    BauraGuiServiceBinding *binding) {
    if (binding == nullptr || binding->service_id == 0) {
        return BAURA_ERR_INVALID_ARGUMENT;
    }
    return baura_service_stop(binding->service_id);
}
