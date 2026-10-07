#include "uart_driver.h"

int uart_init(BauraUart *uart, const BauraUartIo *io) {
    static const struct {
        uint16_t offset;
        uint8_t value;
    } setup[] = {
        {1, 0x00}, {3, 0x80}, {0, 0x03}, {1, 0x00},
        {3, 0x03}, {2, 0xC7}, {4, 0x0B}
    };
    size_t index;
    if (uart == 0 || io == 0 || io->write8 == 0 || io->read8 == 0) {
        return BAURA_UART_ERR_ARGUMENT;
    }
    uart->io = *io;
    uart->initialized = 0;
    for (index = 0; index < sizeof(setup) / sizeof(setup[0]); ++index) {
        if (uart->io.write8(uart->io.context,
                            (uint16_t)(BAURA_UART_COM1 + setup[index].offset),
                            setup[index].value) != 0) {
            return BAURA_UART_ERR_IO;
        }
    }
    uart->initialized = 1;
    return BAURA_UART_OK;
}

int uart_putc(BauraUart *uart, char character) {
    uint32_t poll;
    uint8_t line_status;
    if (uart == 0) return BAURA_UART_ERR_ARGUMENT;
    if (!uart->initialized) return BAURA_UART_ERR_STATE;
    for (poll = 0; poll < BAURA_UART_POLL_LIMIT; ++poll) {
        if (uart->io.read8(uart->io.context, BAURA_UART_COM1 + 5u,
                           &line_status) != 0) {
            return BAURA_UART_ERR_IO;
        }
        if ((line_status & 0x20u) != 0) {
            return uart->io.write8(uart->io.context, BAURA_UART_COM1,
                                   (uint8_t)character) == 0
                       ? BAURA_UART_OK
                       : BAURA_UART_ERR_IO;
        }
    }
    return BAURA_UART_ERR_TIMEOUT;
}

int uart_write(BauraUart *uart, const char *text, size_t length) {
    size_t index;
    if (uart == 0 || (length != 0 && text == 0)) {
        return BAURA_UART_ERR_ARGUMENT;
    }
    for (index = 0; index < length; ++index) {
        int status = uart_putc(uart, text[index]);
        if (status != BAURA_UART_OK) return status;
    }
    return BAURA_UART_OK;
}
