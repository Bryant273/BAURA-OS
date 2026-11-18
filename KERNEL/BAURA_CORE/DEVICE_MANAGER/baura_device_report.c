#include "baura_device_report.h"

const char *baura_device_type_name(DeviceType type) {
    switch (type) {
    case DEV_TYPE_STORAGE: return "storage";
    case DEV_TYPE_NETWORK: return "network";
    case DEV_TYPE_GPU: return "gpu";
    case DEV_TYPE_INPUT: return "input";
    case DEV_TYPE_OTHER: return "other";
    case DEV_TYPE_USB_HOST: return "usb-host";
    case DEV_TYPE_BRIDGE: return "bridge";
    case DEV_TYPE_AUDIO: return "audio";
    case DEV_TYPE_WIRELESS: return "wireless";
    default: return "unknown";
    }
}

static char *put_hex(char *out, uint32_t value, unsigned digits) {
    static const char hex[] = "0123456789abcdef";
    while (digits > 0) {
        --digits;
        *out++ = hex[(value >> (digits * 4u)) & 0xFu];
    }
    return out;
}

static char *put_text(char *out, const char *text) {
    while (*text != '\0') *out++ = *text++;
    return out;
}

size_t baura_device_format_line(const BauraDevice *device, char *buffer, size_t capacity) {
    char *out = buffer;
    if (device == 0 || buffer == 0 || capacity < BAURA_DEVICE_REPORT_LINE_MAX) return 0;
    out = put_text(out, "PCI ");
    out = put_hex(out, device->bus, 2);
    *out++ = ':';
    out = put_hex(out, device->slot, 2);
    *out++ = '.';
    out = put_hex(out, device->function, 1);
    *out++ = ' ';
    out = put_hex(out, device->vendor_id, 4);
    *out++ = ':';
    out = put_hex(out, device->product_id, 4);
    out = put_text(out, " class ");
    out = put_hex(out, device->class_code, 2);
    *out++ = '.';
    out = put_hex(out, device->subclass, 2);
    *out++ = '.';
    out = put_hex(out, device->programming_interface, 2);
    out = put_text(out, " rev ");
    out = put_hex(out, device->revision, 2);
    *out++ = ' ';
    out = put_text(out, baura_device_type_name(device->type));
    *out++ = '\r';
    *out++ = '\n';
    *out = '\0';
    return (size_t)(out - buffer);
}
