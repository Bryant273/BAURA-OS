#include "baura_ps2_sel4.h"

static inline uint8_t in8(seL4_CPtr ioport, uint16_t port) {
    seL4_X86_IOPort_In8_t res = seL4_X86_IOPort_In8(ioport, port);
    return (uint8_t)res.result;
}

static inline void out8(seL4_CPtr ioport, uint16_t port, uint8_t val) {
    (void)seL4_X86_IOPort_Out8(ioport, port, val);
}

static int wait_write(seL4_CPtr ioport) {
    for (int timeout = 0; timeout < 100000; timeout++) {
        if ((in8(ioport, BAURA_PS2_STATUS_PORT) & 0x02u) == 0) return 0;
    }
    return -1;
}

static int wait_read(seL4_CPtr ioport) {
    for (int timeout = 0; timeout < 100000; timeout++) {
        if ((in8(ioport, BAURA_PS2_STATUS_PORT) & 0x01u) != 0) return 0;
    }
    return -1;
}

static void mouse_write(seL4_CPtr ioport, uint8_t byte) {
    wait_write(ioport);
    out8(ioport, BAURA_PS2_STATUS_PORT, 0xD4);
    wait_write(ioport);
    out8(ioport, BAURA_PS2_DATA_PORT, byte);
}

static uint8_t mouse_read(seL4_CPtr ioport) {
    wait_read(ioport);
    return in8(ioport, BAURA_PS2_DATA_PORT);
}

int baura_ps2_sel4_install(BauraPs2Sel4 *ps2, const seL4_BootInfo *boot_info, seL4_SlotPos *next_free_slot) {
    if (ps2 == NULL || boot_info == NULL || next_free_slot == NULL) return -1;
    seL4_SlotPos slot = *next_free_slot;
    if (slot >= boot_info->empty.end) return -1;

    /* Issue IOPort capability for ports 0x60-0x64 */
    seL4_Error err = seL4_X86_IOPortControl_Issue(
        seL4_CapIOPortControl, BAURA_PS2_DATA_PORT, BAURA_PS2_STATUS_PORT,
        seL4_CapInitThreadCNode, slot, seL4_WordBits);
    if (err != seL4_NoError) return -2;

    *next_free_slot = slot + 1;
    ps2->ioport = slot;
    ps2->mouse_cycle = 0;
    ps2->mouse.delta_x = 0;
    ps2->mouse.delta_y = 0;
    ps2->mouse.left_button = 0;
    ps2->mouse.right_button = 0;
    ps2->mouse.middle_button = 0;

    /* Enable auxiliary device (mouse) */
    wait_write(ps2->ioport);
    out8(ps2->ioport, BAURA_PS2_STATUS_PORT, 0xA8);

    /* Configure controller command byte to enable mouse IRQ12 */
    wait_write(ps2->ioport);
    out8(ps2->ioport, BAURA_PS2_STATUS_PORT, 0x20);
    wait_read(ps2->ioport);
    uint8_t status = in8(ps2->ioport, BAURA_PS2_DATA_PORT);
    status |= 0x02; /* Enable mouse interrupt */
    status &= (uint8_t)~0x20; /* Enable mouse clock */
    wait_write(ps2->ioport);
    out8(ps2->ioport, BAURA_PS2_STATUS_PORT, 0x60);
    wait_write(ps2->ioport);
    out8(ps2->ioport, BAURA_PS2_DATA_PORT, status);

    /* Set default mouse settings & enable data reporting */
    mouse_write(ps2->ioport, 0xF6);
    (void)mouse_read(ps2->ioport); /* ACK */

    mouse_write(ps2->ioport, 0xF4);
    (void)mouse_read(ps2->ioport); /* ACK */

    ps2->initialized = 1;
    return 0;
}

int baura_ps2_sel4_poll(BauraPs2Sel4 *ps2, int *out_dx, int *out_dy, uint8_t *out_btn) {
    if (ps2 == NULL || !ps2->initialized) return 0;
    *out_dx = 0;
    *out_dy = 0;
    *out_btn = 0;

    int packets = 0;
    while ((in8(ps2->ioport, BAURA_PS2_STATUS_PORT) & 0x01u) != 0) {
        uint8_t status = in8(ps2->ioport, BAURA_PS2_STATUS_PORT);
        uint8_t data = in8(ps2->ioport, BAURA_PS2_DATA_PORT);

        /* Only process if marked as auxiliary/mouse data */
        if ((status & 0x20u) == 0) continue;

        if (ps2->mouse_cycle == 0) {
            /* Bit 3 of byte 0 must be 1 in standard PS/2 mouse packet */
            if ((data & 0x08u) == 0) continue;
            ps2->mouse_bytes[0] = data;
            ps2->mouse_cycle = 1;
        } else if (ps2->mouse_cycle == 1) {
            ps2->mouse_bytes[1] = data;
            ps2->mouse_cycle = 2;
        } else if (ps2->mouse_cycle == 2) {
            ps2->mouse_bytes[2] = data;
            ps2->mouse_cycle = 0;

            const uint8_t flags = ps2->mouse_bytes[0];
            int16_t dx = (int16_t)ps2->mouse_bytes[1];
            int16_t dy = (int16_t)ps2->mouse_bytes[2];

            /* Sign extensions */
            if ((flags & 0x10u) != 0) dx = (int16_t)(dx - 256);
            if ((flags & 0x20u) != 0) dy = (int16_t)(dy - 256);

            /* Discard overflow packets */
            if ((flags & 0xC0u) == 0) {
                *out_dx += (int)dx;
                *out_dy -= (int)dy; /* Invert Y axis for screen space */
                *out_btn = flags & 0x07u;
                packets++;
            }
        }
    }
    return packets;
}
