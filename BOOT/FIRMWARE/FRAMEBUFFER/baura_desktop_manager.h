#ifndef BAURA_DESKTOP_MANAGER_H
#define BAURA_DESKTOP_MANAGER_H

#include <stdint.h>
#include <stddef.h>
#include "baura_boot_splash.h"

#define BAURA_MAX_WINDOWS 4

typedef struct {
    int id;
    int x;
    int y;
    int w;
    int h;
    int visible;
    int focused;
    int dragging;
    int drag_dx;
    int drag_dy;
    const char *title;
    uint32_t icon_color;
} BauraWindowItem;

typedef struct {
    int mouse_x;
    int mouse_y;
    uint8_t mouse_buttons;
    uint8_t prev_buttons;

    int bstart_open;
    int focused_window_id;

    BauraWindowItem windows[BAURA_MAX_WINDOWS];
    int window_count;

    /* Background backup under mouse cursor (16x20 pixels) */
    uint32_t cursor_saved_bg[16 * 20];
    int cursor_saved_x;
    int cursor_saved_y;
    int cursor_saved_w;
    int cursor_saved_h;
    int cursor_has_backup;
} BauraDesktopManager;

void baura_desktop_init(BauraDesktopManager *dm, int screen_w, int screen_h);
void baura_desktop_render_full(const BauraBootFramebuffer *fb, BauraDesktopManager *dm);
void baura_desktop_handle_mouse(const BauraBootFramebuffer *fb, BauraDesktopManager *dm, int dx, int dy, uint8_t buttons);

#endif /* BAURA_DESKTOP_MANAGER_H */
