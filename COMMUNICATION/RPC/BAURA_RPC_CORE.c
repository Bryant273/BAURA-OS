#include "BAURA_RPC_CORE.h"

int baura_rpc_call(const BauraRPC_Transport *transport, uint64_t endpoint,
                   uint32_t interface_id, uint32_t method_id,
                   const uint64_t args[BAURA_RPC_ARGUMENT_COUNT],
                   uint64_t timeout_ms, BauraRPC_Response *response) {
    BauraRPC_Request request;
    BauraRPC_Response result = {0};
    uint32_t index;

    if (transport == 0 || endpoint == 0 || interface_id == 0 ||
        method_id == 0 || args == 0 || response == 0) {
        return BAURA_RPC_ERR_ARGUMENT;
    }
    if (transport->exchange == 0) return BAURA_RPC_ERR_UNAVAILABLE;

    request.interface_id = interface_id;
    request.method_id = method_id;
    for (index = 0; index < BAURA_RPC_ARGUMENT_COUNT; ++index) {
        request.args[index] = args[index];
    }

    if (transport->exchange(transport->context, endpoint, &request, &result,
                           timeout_ms) != 0) {
        return BAURA_RPC_ERR_TRANSPORT;
    }
    *response = result;
    return BAURA_RPC_OK;
}
