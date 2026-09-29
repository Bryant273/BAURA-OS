
#include <stdint.h>
#include "net_core.h"

// API 162: BAURA_NET_CONNECT (Detailed Handshake)
int api_net_connect(int fd, uint32_t ip, uint16_t port) {
    NetSocket* sock = get_socket(fd);
    if (!sock) return ERR_INVALID_SOCKET;

    // 1. Transition to SYN_SENT
    sock->remote_ip = ip;
    sock->remote_port = port;
    sock->state = NET_SYN_SENT;

    // 2. Construct TCP SYN Packet
    TCPPacket pkt = {
        .src_port = sock->local_port,
        .dst_port = port,
        .seq = generate_random_seq(),
        .flags = FLAG_SYN
    };

    // 3. Send via Hardware Driver (e1000/rtl8168)
    if (net_hw_send_packet(sock->remote_ip, &pkt) != SUCCESS) {
        sock->state = NET_CLOSED;
        return ERR_NETWORK_UNREACHABLE;
    }

    // 4. Wait for SYN-ACK with a timeout
    if (net_wait_for_packet(sock, FLAG_SYN | FLAG_ACK, 5000)) {
        sock->state = NET_ESTABLISHED;
        return SUCCESS;
    }

    sock->state = NET_CLOSED;
    return ERR_TIMEOUT;
}
