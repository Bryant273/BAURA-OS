#ifndef BAURA_PS2_DRIVER_H
#define BAURA_PS2_DRIVER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BAURA_PS2_DATA_PORT 0x60u
#define BAURA_PS2_STATUS_PORT 0x64u
#define BAURA_PS2_POLL_LIMIT 100000u

typedef int (*BauraPs2ReadPort)(void *context, uint16_t port,
                                uint8_t *value);
typedef int (*BauraPs2WritePort)(void *context, uint16_t port,
                                 uint8_t value);

typedef struct {
    BauraPs2ReadPort read8;
    BauraPs2WritePort write8;
    void *context;
} BauraPs2Io;

typedef enum {
    BAURA_PS2_EVENT_KEY = 1,
    BAURA_PS2_EVENT_MOUSE = 2
} BauraPs2EventType;

typedef struct {
    BauraPs2EventType type;
    union {
        struct {
            uint16_t scan_code;
            uint8_t ascii;
            uint8_t pressed;
            uint8_t modifiers;
        } key;
        struct {
            int16_t delta_x;
            int16_t delta_y;
            uint8_t buttons;
        } mouse;
    } data;
} BauraPs2Event;

typedef struct {
    BauraPs2Io io;
    uint8_t initialized;
    uint8_t extended;
    uint8_t key_released;
    uint8_t left_shift;
    uint8_t right_shift;
    uint8_t caps_lock;
    uint8_t mouse_packet[3];
    uint8_t mouse_packet_length;
} BauraPs2Driver;

enum {
    BAURA_PS2_OK = 0,
    BAURA_PS2_ERR_ARGUMENT = -1,
    BAURA_PS2_ERR_IO = -2,
    BAURA_PS2_ERR_TIMEOUT = -3,
    BAURA_PS2_ERR_DEVICE = -4,
    BAURA_PS2_ERR_STATE = -5
};

int baura_ps2_init(BauraPs2Driver *driver, const BauraPs2Io *io);
int baura_ps2_poll(BauraPs2Driver *driver, BauraPs2Event *events,
                   uint32_t capacity, uint32_t *event_count);

#ifdef __cplusplus
}
#endif

#endif
