#include "baura_uart_console.h"

#include "../../../BOOT/FIRMWARE/UART/uart_driver.h"

BauraStatus baura_core_uart_console_write(void *context, const char *text,
                                          size_t length) {
    int status;
    if (context == 0 || (length != 0 && text == 0)) {
        return BAURA_ERR_INVALID_ARGUMENT;
    }
    status = uart_write((BauraUart *)context, text, length);
    if (status == BAURA_UART_OK) return BAURA_OK;
    if (status == BAURA_UART_ERR_ARGUMENT) return BAURA_ERR_INVALID_ARGUMENT;
    if (status == BAURA_UART_ERR_STATE) return BAURA_ERR_INVALID_STATE;
    return status == BAURA_UART_ERR_TIMEOUT ? BAURA_ERR_LIMIT : BAURA_ERR_IO;
}
