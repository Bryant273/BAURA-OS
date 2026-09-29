
#ifndef BAURA_LOG_H
#define BAURA_LOG_H

#include "../../BOOT/FIRMWARE/UART/uart_driver.h"
#include <stdarg.h>

typedef enum {
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_CRITICAL,
    LOG_DEBUG
} LogLevel;

void baura_log(LogLevel level, const char* fmt, ...);

#define LOG_I(...) baura_log(LOG_INFO, __VA_ARGS__)
#define LOG_W(...) baura_log(LOG_WARN, __VA_ARGS__)
#define LOG_E(...) baura_log(LOG_ERROR, __VA_ARGS__)
#define LOG_C(...) baura_log(LOG_CRITICAL, __VA_ARGS__)
#define LOG_D(...) baura_log(LOG_DEBUG, __VA_ARGS__)

#endif
