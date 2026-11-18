#ifndef BAURA_PS2_SEL4_H
#define BAURA_PS2_SEL4_H

#include <stdint.h>
#include <stddef.h>
#include <sel4/sel4.h>

#define BAURA_PS2_DATA_PORT   0x60u
#define BAURA_PS2_STATUS_PORT 0x64u

typedef struct {
    int16_t delta_x;
    int16_t delta_y;
    uint8_t left_button;
    uint8_t right_button;
    uint8_t middle_button;
} BauraMouseState;

typedef struct {
    seL4_CPtr ioport;
    uint8_t mouse_cycle;
    uint8_t mouse_bytes[3];
    BauraMouseState mouse;
    int initialized;
} BauraPs2Sel4;

int baura_ps2_sel4_install(BauraPs2Sel4 *ps2, const seL4_BootInfo *boot_info, seL4_SlotPos *next_free_slot);
int baura_ps2_sel4_poll(BauraPs2Sel4 *ps2, int *out_dx, int *out_dy, uint8_t *out_btn);

#endif /* BAURA_PS2_SEL4_H */
