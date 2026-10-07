#ifndef BAURA_UART_CONSOLE_H
#define BAURA_UART_CONSOLE_H

#include "baura_core_entry.h"

BauraStatus baura_core_uart_console_write(void *context, const char *text,
                                          size_t length);

#endif
