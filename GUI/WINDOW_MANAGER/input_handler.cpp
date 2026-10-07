#include "input_handler.h"

int handle_global_shortcuts(uint32_t key_mask,
                            BauraSecurityOverlayRequest request,
                            void *context) {
    if ((key_mask & BAURA_SHORTCUT_SECURITY_OVERLAY_MASK) !=
        BAURA_SHORTCUT_SECURITY_OVERLAY_MASK) {
        return BAURA_SHORTCUT_NOT_HANDLED;
    }
    if (request == nullptr) return BAURA_SHORTCUT_ERR_UNAVAILABLE;
    return request(context);
}
