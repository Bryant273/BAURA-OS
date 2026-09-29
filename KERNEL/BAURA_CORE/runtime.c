#include <stddef.h>

void *memcpy(void *destination, const void *source, size_t count) {
    unsigned char *out = (unsigned char *)destination;
    const unsigned char *in = (const unsigned char *)source;
    size_t index;
    for (index = 0; index < count; ++index) out[index] = in[index];
    return destination;
}

void *memset(void *destination, int value, size_t count) {
    unsigned char *out = (unsigned char *)destination;
    size_t index;
    for (index = 0; index < count; ++index) out[index] = (unsigned char)value;
    return destination;
}

size_t strlen(const char *text) {
    size_t length = 0;
    if (text == 0) return 0;
    while (text[length] != '\0') ++length;
    return length;
}
