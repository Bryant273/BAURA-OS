#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "../FIRMWARE/UART/uart_driver.h"
#include "../../KERNEL/BAURA_CORE/SYSTEM_API/baura_uart_console.h"

typedef struct {
    uint32_t writes;
    uint32_t reads;
    uint8_t ready;
    char output[16];
    uint32_t output_length;
} FakeUart;

static int write_port(void *context, uint16_t port, uint8_t value) {
    FakeUart *fake = (FakeUart *)context;
    ++fake->writes;
    if (port == BAURA_UART_COM1 && fake->ready) {
        fake->output[fake->output_length++] = (char)value;
    }
    return 0;
}

static int read_port(void *context, uint16_t port, uint8_t *value) {
    FakeUart *fake = (FakeUart *)context;
    assert(port == BAURA_UART_COM1 + 5u);
    ++fake->reads;
    *value = fake->ready ? 0x20 : 0;
    return 0;
}

int main(void) {
    FakeUart fake = {0};
    BauraUart uart = {0};
    BauraUartIo io = {write_port, read_port, &fake};
    assert(uart_putc(&uart, 'x') == BAURA_UART_ERR_STATE);
    assert(uart_init(&uart, &io) == BAURA_UART_OK);
    assert(fake.writes == 7);
    assert(uart_write(&uart, "BAURA", 5) == BAURA_UART_ERR_TIMEOUT);
    fake.ready = 1;
    assert(uart_write(&uart, "BAURA", 5) == BAURA_UART_OK);
    assert(fake.output_length == 5);
    assert(memcmp(fake.output, "BAURA", 5) == 0);
    assert(baura_core_uart_console_write(&uart, "Core", 4) == BAURA_OK);
    assert(fake.output_length == 9);
    assert(memcmp(fake.output + 5, "Core", 4) == 0);
    assert(uart_write(&uart, 0, 1) == BAURA_UART_ERR_ARGUMENT);
    assert(baura_core_uart_console_write(0, "Core", 4) ==
           BAURA_ERR_INVALID_ARGUMENT);
    return 0;
}
