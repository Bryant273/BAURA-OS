#ifndef BAURA_RPC_CORE_H
#define BAURA_RPC_CORE_H

#include <stdint.h>
#include "../../03_KERNEL/L4/08_IPC/MESSAGE_PASSING.h"

typedef struct {
    uint32_t interface_id;
    uint32_t method_id;
    uint64_t args[4];
} BauraRPC_Request;

typedef struct {
    uint32_t status;
    uint64_t result[4];
} BauraRPC_Response;

// Envoie une requête RPC et attend la réponse
int baura_rpc_call(Capability server_cap, uint32_t method_id, void* args, void* response);

#endif