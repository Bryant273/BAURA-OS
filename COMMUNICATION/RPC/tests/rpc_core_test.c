#include <assert.h>
#include <stdint.h>
#include "../BAURA_RPC_CORE.h"

static int exchange_result;
static uint32_t exchanges;

static int exchange(void *context, uint64_t endpoint,
                    const BauraRPC_Request *request,
                    BauraRPC_Response *response, uint64_t timeout_ms) {
    uint32_t *calls = (uint32_t *)context;
    assert(endpoint == 7);
    assert(request->interface_id == 2 && request->method_id == 3);
    assert(request->args[0] == 11 && request->args[3] == 14);
    assert(timeout_ms == 500);
    ++*calls;
    response->status = 9;
    response->result[0] = 99;
    return exchange_result;
}

int main(void) {
    const uint64_t args[BAURA_RPC_ARGUMENT_COUNT] = {11, 12, 13, 14};
    BauraRPC_Response response = {0};
    BauraRPC_Transport transport = {exchange, &exchanges};

    assert(baura_rpc_call(&transport, 7, 2, 3, args, 500, &response) ==
           BAURA_RPC_OK);
    assert(exchanges == 1 && response.status == 9 && response.result[0] == 99);
    response.status = 42;
    exchange_result = -1;
    assert(baura_rpc_call(&transport, 7, 2, 3, args, 500, &response) ==
           BAURA_RPC_ERR_TRANSPORT);
    assert(response.status == 42 && exchanges == 2);
    assert(baura_rpc_call(&transport, 0, 2, 3, args, 500, &response) ==
           BAURA_RPC_ERR_ARGUMENT);
    transport.exchange = 0;
    assert(baura_rpc_call(&transport, 7, 2, 3, args, 500, &response) ==
           BAURA_RPC_ERR_UNAVAILABLE);
    return 0;
}
