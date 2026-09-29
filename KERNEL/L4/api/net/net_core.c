
#include <stdint.h>
#include "L4_TCB.h"

typedef enum { NET_CLOSED, NET_LISTENING, NET_ESTABLISHED, NET_CLOSING } NetState;

typedef struct {
    uint32_t local_ip;
    uint32_t remote_ip;
    uint16_t local_port;
    uint16_t remote_port;
    NetState state;
    uint32_t seq_num;
    uint32_t ack_num;
} NetSocket;

// API 151: BAURA_NET_SOCKET
int api_net_socket(int domain, int type, int protocol) {
    if (domain != AF_INET) return ERR_UNSUPPORTED_DOMAIN;

    NetSocket* sock = slab_alloc(sizeof(NetSocket));
    if (!sock) return ERR_OUT_OF_MEMORY;

    sock->state = NET_CLOSED;
    sock->local_port = net_find_free_port();

    return assign_socket_fd(sock);
}
