#ifndef BAURA_RPC_CORE_H
#define BAURA_RPC_CORE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BAURA_RPC_ARGUMENT_COUNT 4u

typedef struct {
    uint32_t interface_id;
    uint32_t method_id;
    uint64_t args[BAURA_RPC_ARGUMENT_COUNT];
} BauraRPC_Request;

typedef struct {
    uint32_t status;
    uint32_t reserved;
    uint64_t result[BAURA_RPC_ARGUMENT_COUNT];
} BauraRPC_Response;

typedef int (*BauraRPC_Exchange)(void *context, uint64_t endpoint,
                                 const BauraRPC_Request *request,
                                 BauraRPC_Response *response,
                                 uint64_t timeout_ms);

typedef struct {
    BauraRPC_Exchange exchange;
    void *context;
} BauraRPC_Transport;

enum {
    BAURA_RPC_OK = 0,
    BAURA_RPC_ERR_ARGUMENT = -1,
    BAURA_RPC_ERR_UNAVAILABLE = -2,
    BAURA_RPC_ERR_TRANSPORT = -3
};

int baura_rpc_call(const BauraRPC_Transport *transport, uint64_t endpoint,
                   uint32_t interface_id, uint32_t method_id,
                   const uint64_t args[BAURA_RPC_ARGUMENT_COUNT],
                   uint64_t timeout_ms, BauraRPC_Response *response);

#ifdef __cplusplus
}
#endif

#endif
