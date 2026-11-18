#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../FIRMWARE/FRAMEBUFFER/baura_desktop_manager.h"

int main(void) {
    uint32_t buffer[1024 * 768];
    BauraBootFramebuffer fb;
    memset(&fb, 0, sizeof(fb));
    fb.pixels = buffer;
    fb.width = 1024;
    fb.height = 768;
    fb.pitch = 1024 * 4;
    fb.bpp = 32;
    fb.format = BAURA_BOOT_FB_FORMAT_RGBX;

    BauraDesktopManager dm;
    baura_desktop_init(&dm, 1024, 768);

    /* 1. Verify clean non-overlapping initial layout */
    assert(dm.window_count == 2);
    /* Window 0 (Fichiers) */
    assert(dm.windows[0].x == 48);
    assert(dm.windows[0].y == 70);
    assert(dm.windows[0].w == 440);
    /* Window 1 (Terminal) */
    assert(dm.windows[1].x == 520);
    assert(dm.windows[1].y == 70);
    assert(dm.windows[1].w == 456);

    /* Verify 32px gap between windows (NO OVERLAPPING!) */
    assert((dm.windows[0].x + dm.windows[0].w) < dm.windows[1].x);
    assert(dm.windows[1].x - (dm.windows[0].x + dm.windows[0].w) == 32);

    /* 2. Test full render without crashes */
    memset(buffer, 0, sizeof(buffer));
    baura_desktop_render_full(&fb, &dm);

    int non_zero = 0;
    for (size_t i = 0; i < 1024 * 768; ++i) {
        if (buffer[i] != 0) ++non_zero;
    }
    assert(non_zero > 50000);

    /* 3. Test mouse move */
    int initial_mx = dm.mouse_x;
    int initial_my = dm.mouse_y;
    baura_desktop_handle_mouse(&fb, &dm, 20, -10, 0);
    assert(dm.mouse_x == initial_mx + 20);
    assert(dm.mouse_y == initial_my - 10);

    /* 4. Test titlebar drag */
    /* Click on Terminal titlebar at (600, 80) */
    dm.mouse_x = 600;
    dm.mouse_y = 80;
    baura_desktop_handle_mouse(&fb, &dm, 0, 0, 0x01); /* Left click down */
    assert(dm.windows[1].dragging == 1);
    assert(dm.focused_window_id == 1);

    /* Drag by dx=30, dy=40 */
    baura_desktop_handle_mouse(&fb, &dm, 30, 40, 0x01);
    assert(dm.windows[1].x == 520 + 30);
    assert(dm.windows[1].y == 70 + 40);

    /* Release mouse */
    baura_desktop_handle_mouse(&fb, &dm, 0, 0, 0x00);
    assert(dm.windows[1].dragging == 0);

    /* 5. Test close button on Fichiers window */
    /* Fichiers close button is at (w->x + 12, w->y + 11) -> (48 + 16, 70 + 14) = (64, 84) */
    dm.mouse_x = 48 + 16;
    dm.mouse_y = 70 + 14;
    baura_desktop_handle_mouse(&fb, &dm, 0, 0, 0x01); /* Click close */
    assert(dm.windows[0].visible == 0);

    /* 6. Test dock restore on Fichiers icon */
    /* Fichiers dock icon is at x=78..112, y=710 */
    dm.mouse_x = 90;
    dm.mouse_y = 720;
    baura_desktop_handle_mouse(&fb, &dm, 0, 0, 0x00); /* release */
    baura_desktop_handle_mouse(&fb, &dm, 0, 0, 0x01); /* click */
    assert(dm.windows[0].visible == 1);

    return 0;
}
