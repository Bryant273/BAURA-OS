
#include "baura_log.h"
#include <stdio.h>

void baura_log(LogLevel level, const char* fmt, ...) {
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    const char* level_str;
    switch(level) {
        case LOG_INFO:    level_str = "[INFO]"; break;
        case LOG_WARN:    level_str = "[WARN]"; break;
        case LOG_ERROR:   level_str = "[ERR ]"; break;
        case LOG_CRITICAL: level_str = "[CRIT]"; break;
        case LOG_DEBUG:   level_str = "[DEBG]"; break;
        default:          level_str = "[LOG ]"; break;
    }

    uart_puts(level_str);
    uart_puts(" ");
    uart_puts(buffer);
    uart_puts("\n");
}
