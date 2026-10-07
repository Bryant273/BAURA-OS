#ifndef BAURA_UART_H
#define BAURA_UART_H

#include <stddef.h>
#include <stdint.h>

#define BAURA_UART_COM1 0x3F8u
#define BAURA_UART_DIVISOR_115200 1u
#define BAURA_UART_POLL_LIMIT 100000u

typedef int (*BauraPortWrite8)(void *context, uint16_t port, uint8_t value);
typedef int (*BauraPortRead8)(void *context, uint16_t port, uint8_t *value);

typedef struct {
    BauraPortWrite8 write8;
    BauraPortRead8 read8;
    void *context;
} BauraUartIo;

typedef struct {
    BauraUartIo io;
    uint8_t initialized;
} BauraUart;

enum {
    BAURA_UART_OK = 0,
    BAURA_UART_ERR_ARGUMENT = -1,
    BAURA_UART_ERR_IO = -2,
    BAURA_UART_ERR_TIMEOUT = -3,
    BAURA_UART_ERR_STATE = -4
};

int uart_init(BauraUart *uart, const BauraUartIo *io);
int uart_putc(BauraUart *uart, char character);
int uart_write(BauraUart *uart, const char *text, size_t length);

#endif
