#ifndef BAURA_NETWORK_H
#define BAURA_NETWORK_H

#include "../baura_core_types.h"

#define BAURA_NETWORK_FRAME_MAX 1518u
typedef BauraStatus (*BauraNetworkTransmit)(void *context, const uint8_t *frame, uint32_t length);

void network_init(void);
BauraStatus baura_network_set_backend(BauraNetworkTransmit transmit, void *context, uint32_t mtu);
BauraStatus baura_network_send_frame(const uint8_t *frame, uint32_t length);
uint32_t baura_network_mtu(void);

#endif
