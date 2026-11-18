#ifndef BAURA_DEVICE_REPORT_H
#define BAURA_DEVICE_REPORT_H

#include <stddef.h>
#include "BAURA_DEVICE_MGR.h"

/* Smallest buffer that baura_device_format_line() accepts. */
#define BAURA_DEVICE_REPORT_LINE_MAX 64u

const char *baura_device_type_name(DeviceType type);

/*
 * Formats one console line, lspci-style and without libc, e.g.
 *   "PCI 00:03.0 8086:100e class 02.00.00 rev 03 network\r\n"
 * Returns the number of characters written (not counting the terminating NUL),
 * or 0 when the arguments are invalid or the buffer is too small.
 */
size_t baura_device_format_line(const BauraDevice *device, char *buffer, size_t capacity);

#endif
