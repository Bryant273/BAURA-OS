#include "baura_network.h"

static BauraNetworkTransmit transmit_frame;
static void *backend_context;
static uint32_t current_mtu;

void network_init(void) { transmit_frame = 0; backend_context = 0; current_mtu = 0; }

BauraStatus baura_network_set_backend(BauraNetworkTransmit transmit, void *context, uint32_t mtu) {
    if (transmit == 0 || mtu < 14 || mtu > BAURA_NETWORK_FRAME_MAX) return BAURA_ERR_INVALID_ARGUMENT;
    transmit_frame = transmit; backend_context = context; current_mtu = mtu; return BAURA_OK;
}

BauraStatus baura_network_send_frame(const uint8_t *frame, uint32_t length) {
    if (frame == 0 || length < 14) return BAURA_ERR_INVALID_ARGUMENT;
    if (transmit_frame == 0) return BAURA_ERR_INVALID_STATE;
    if (length > current_mtu) return BAURA_ERR_LIMIT;
    return transmit_frame(backend_context, frame, length);
}

uint32_t baura_network_mtu(void) { return current_mtu; }
