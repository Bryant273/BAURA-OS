#ifndef BAURA_GLOBAL_INPUT_HANDLER_H
#define BAURA_GLOBAL_INPUT_HANDLER_H

#include <stdint.h>

#define BAURA_SHORTCUT_SECURITY_OVERLAY_MASK UINT32_C(0x7)

typedef int (*BauraSecurityOverlayRequest)(void *context);

enum {
    BAURA_SHORTCUT_NOT_HANDLED = 1,
    BAURA_SHORTCUT_ERR_ARGUMENT = -1,
    BAURA_SHORTCUT_ERR_UNAVAILABLE = -2
};

int handle_global_shortcuts(uint32_t key_mask,
                            BauraSecurityOverlayRequest request,
                            void *context);

#endif
