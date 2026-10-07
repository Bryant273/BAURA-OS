#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "../BAURA_MESSAGE_BUS.h"

static uint32_t first_calls;
static uint32_t second_calls;
static uint32_t second_subscription;

static void first_subscriber(const BusMessage *message, void *context) {
    uint32_t *calls = (uint32_t *)context;
    assert(message->topic_id == 5);
    assert(message->size == 4);
    assert(memcmp(message->data, "test", 4) == 0);
    ++*calls;
    assert(bus_unsubscribe(second_subscription) == BAURA_BUS_OK);
}

static void second_subscriber(const BusMessage *message, void *context) {
    uint32_t *calls = (uint32_t *)context;
    assert(message->topic_id == 5);
    ++*calls;
}

int main(void) {
    uint32_t first_subscription;
    assert(bus_subscribe(5, first_subscriber, &first_calls,
                         &first_subscription) == BAURA_BUS_OK);
    assert(bus_subscribe(5, second_subscriber, &second_calls,
                         &second_subscription) == BAURA_BUS_OK);
    assert(bus_publish(5, "test", 4) == BAURA_BUS_OK);
    assert(first_calls == 1 && second_calls == 1);
    assert(bus_publish(5, "test", 4) == BAURA_BUS_OK);
    assert(first_calls == 2 && second_calls == 1);
    assert(bus_publish(0, 0, 0) == BAURA_BUS_ERR_ARGUMENT);
    assert(bus_unsubscribe(first_subscription) == BAURA_BUS_OK);
    assert(bus_unsubscribe(first_subscription) == BAURA_BUS_ERR_NOT_FOUND);
    return 0;
}
