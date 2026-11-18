#include "baura_desktop_manager.h"
#include "baura_inter_font.h"

/* Fast integer rounded rectangle */
static void draw_rrect_fast(const BauraBootFramebuffer *fb, int x, int y, int w, int h, int r, uint32_t color) {
    if (fb == NULL || fb->pixels == NULL || w <= 0 || h <= 0) return;
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;

    baura_boot_fill_rect(fb, x + r, y, w - 2 * r, h, color);
    baura_boot_fill_rect(fb, x, y + r, r, h - 2 * r, color);
    baura_boot_fill_rect(fb, x + w - r, y + r, r, h - 2 * r, color);

    const int r2 = r * r;
    for (int cy = 0; cy < r; ++cy) {
        const int dy = r - 1 - cy;
        for (int cx = 0; cx < r; ++cx) {
            const int dx = r - 1 - cx;
            if (dx * dx + dy * dy <= r2) {
                baura_boot_draw_pixel(fb, x + cx, y + cy, color);
                baura_boot_draw_pixel(fb, x + w - 1 - cx, y + cy, color);
                baura_boot_draw_pixel(fb, x + cx, y + h - 1 - cy, color);
                baura_boot_draw_pixel(fb, x + w - 1 - cx, y + h - 1 - cy, color);
            }
        }
    }
}

/* Mouse cursor arrow bitmap (16x18) */
static const char *cursor_bitmap[18] = {
    "X               ",
    "XX              ",
    "X.X             ",
    "X..X            ",
    "X...X           ",
    "X....X          ",
    "X.....X         ",
    "X......X        ",
    "X.......X       ",
    "X........X      ",
    "X.....XXXX      ",
    "X..X..X         ",
    "X.X X..X        ",
    "XX   X..X       ",
    "X     X..X      ",
    "       X..X     ",
    "        XX      ",
    "                "
};

static void cursor_restore(const BauraBootFramebuffer *fb, BauraDesktopManager *dm) {
    if (!dm->cursor_has_backup || fb == NULL || fb->pixels == NULL) return;
    for (int y = 0; y < dm->cursor_saved_h; ++y) {
        int py = dm->cursor_saved_y + y;
        if (py < 0 || (uint32_t)py >= fb->height) continue;
        for (int x = 0; x < dm->cursor_saved_w; ++x) {
            int px = dm->cursor_saved_x + x;
            if (px < 0 || (uint32_t)px >= fb->width) continue;
            baura_boot_draw_pixel(fb, px, py, dm->cursor_saved_bg[y * 16 + x]);
        }
    }
    dm->cursor_has_backup = 0;
}

static void cursor_draw(const BauraBootFramebuffer *fb, BauraDesktopManager *dm) {
    if (fb == NULL || fb->pixels == NULL) return;
    dm->cursor_saved_x = dm->mouse_x;
    dm->cursor_saved_y = dm->mouse_y;
    dm->cursor_saved_w = 16;
    dm->cursor_saved_h = 18;

    for (int y = 0; y < 18; ++y) {
        int py = dm->mouse_y + y;
        for (int x = 0; x < 16; ++x) {
            int px = dm->mouse_x + x;
            if (px >= 0 && (uint32_t)px < fb->width && py >= 0 && (uint32_t)py < fb->height) {
                volatile uint32_t *line = (volatile uint32_t *)((uint8_t *)fb->pixels + py * fb->pitch);
                dm->cursor_saved_bg[y * 16 + x] = line[px];
            } else {
                dm->cursor_saved_bg[y * 16 + x] = 0;
            }
        }
    }
    dm->cursor_has_backup = 1;

    for (int y = 0; y < 18; ++y) {
        int py = dm->mouse_y + y;
        if (py < 0 || (uint32_t)py >= fb->height) continue;
        for (int x = 0; x < 16; ++x) {
            int px = dm->mouse_x + x;
            if (px < 0 || (uint32_t)px >= fb->width) continue;
            char c = cursor_bitmap[y][x];
            if (c == 'X') {
                baura_boot_draw_pixel(fb, px, py, 0xFF0A071E);
            } else if (c == '.') {
                baura_boot_draw_pixel(fb, px, py, 0xFFFFFFFF);
            }
        }
    }
}

void baura_desktop_init(BauraDesktopManager *dm, int screen_w, int screen_h) {
    if (dm == NULL) return;
    dm->mouse_x = screen_w / 2;
    dm->mouse_y = screen_h / 2;
    dm->mouse_buttons = 0;
    dm->prev_buttons = 0;
    dm->bstart_open = 0;
    dm->cursor_has_backup = 0;
    dm->window_count = 2;

    /* Window 0: Fichiers — Left Side, NO OVERLAPPING */
    dm->windows[0].id = 0;
    dm->windows[0].x = 48;
    dm->windows[0].y = 70;
    dm->windows[0].w = 440;
    dm->windows[0].h = 420;
    dm->windows[0].visible = 1;
    dm->windows[0].focused = 0;
    dm->windows[0].dragging = 0;
    dm->windows[0].title = "Fichiers \xe2\x80\x94 Documents";
    dm->windows[0].icon_color = 0xFF4F46E5;

    /* Window 1: Terminal — Right Side, 32px gap, NO OVERLAPPING */
    dm->windows[1].id = 1;
    dm->windows[1].x = 520;
    dm->windows[1].y = 70;
    dm->windows[1].w = 456;
    dm->windows[1].h = 420;
    dm->windows[1].visible = 1;
    dm->windows[1].focused = 1;
    dm->windows[1].dragging = 0;
    dm->windows[1].title = "Terminal \xe2\x80\x94 baura@x86_64";
    dm->windows[1].icon_color = 0xFF10B981;

    dm->focused_window_id = 1;
}

static void draw_window_frame(const BauraBootFramebuffer *fb, const BauraWindowItem *win) {
    if (!win->visible) return;
    const int x = win->x;
    const int y = win->y;
    const int w = win->w;
    const int h = win->h;

    /* Shadow */
    draw_rrect_fast(fb, x - 2, y + 4, w + 4, h + 4, 14, 0x30000000);

    /* Main Window Background */
    const uint32_t bg_col = (win->id == 1) ? 0xFF0D0F18 : 0xFF141032;
    draw_rrect_fast(fb, x, y, w, h, 12, bg_col);

    /* Titlebar Native 34px */
    const uint32_t tb_col = win->focused ? ((win->id == 1) ? 0xFF1C2236 : 0xFF28205C) : 0xFF181438;
    draw_rrect_fast(fb, x, y, w, 34, 12, tb_col);
    baura_boot_fill_rect(fb, x, y + 20, w, 14, tb_col); /* Fill lower radius */

    /* Window Controls (Red=Close, Yellow=Min, Green=Max) */
    draw_rrect_fast(fb, x + 12, y + 11, 12, 12, 6, 0xFFEF4444);
    draw_rrect_fast(fb, x + 30, y + 11, 12, 12, 6, 0xFFF59E0B);
    draw_rrect_fast(fb, x + 48, y + 11, 12, 12, 6, 0xFF10B981);

    /* Icon tile & Title */
    draw_rrect_fast(fb, x + 70, y + 8, 18, 18, 5, win->icon_color);
    draw_inter_string_14(fb, x + 96, y + 23, win->title, win->focused ? 0xFFF4F2FF : 0xFF9CA3AF);

    /* Content Area */
    if (win->id == 0) {
        /* Fichiers Window Content */
        /* Left Sidebar */
        baura_boot_fill_rect(fb, x, y + 34, 120, h - 34, 0xFF18133B);
        baura_boot_fill_rect(fb, x + 120, y + 34, 1, h - 34, 0xFF2B225C);
        draw_inter_string_14(fb, x + 12, y + 60, "> Documents", 0xFF6D66F2);
        draw_inter_string_14(fb, x + 12, y + 88, "  Images", 0xFFB4B0DA);
        draw_inter_string_14(fb, x + 12, y + 116, "  Musique", 0xFFB4B0DA);
        draw_inter_string_14(fb, x + 12, y + 144, "  Disque PC:", 0xFFB4B0DA);

        /* Main View */
        draw_rrect_fast(fb, x + 136, y + 54, 90, 68, 10, 0xFF231B54);
        draw_rrect_fast(fb, x + 146, y + 64, 24, 18, 5, 0xFF4F46E5);
        draw_inter_string_14(fb, x + 146, y + 108, "Projets", 0xFFF4F2FF);

        draw_rrect_fast(fb, x + 236, y + 54, 90, 68, 10, 0xFF231B54);
        draw_rrect_fast(fb, x + 246, y + 64, 24, 18, 5, 0xFF8B5CF6);
        draw_inter_string_14(fb, x + 246, y + 108, "Factures", 0xFFF4F2FF);

        draw_rrect_fast(fb, x + 336, y + 54, 90, 68, 10, 0xFF231B54);
        draw_rrect_fast(fb, x + 346, y + 64, 24, 18, 5, 0xFFD0AE2A);
        draw_inter_string_14(fb, x + 346, y + 108, "Systeme", 0xFFF4F2FF);

        /* Status line */
        baura_boot_fill_rect(fb, x + 121, y + h - 26, w - 121, 1, 0xFF2B225C);
        draw_inter_string_14(fb, x + 130, y + h - 8, "3 \xc3\xa9l\xc3\xa9ments \xc2\xb7 Pr\xc3\xaat", 0xFF8E8AA8);
    } else if (win->id == 1) {
        /* Terminal Window Content */
        int ty = y + 60;
        draw_inter_string_14(fb, x + 16, ty, "BAURA OS 1.0 (x86_64 pc99) \xe2\x80\x94 seL4 16.0", 0xFF10B981);
        ty += 24;
        draw_inter_string_14(fb, x + 16, ty, "baura@x86_64:~$ uname -a", 0xFFF4F2FF);
        ty += 20;
        draw_inter_string_14(fb, x + 16, ty, "BAURA-OS pc99 1.0.0-seL4 #1 SMP x86_64", 0xFFB4B0DA);
        ty += 24;
        draw_inter_string_14(fb, x + 16, ty, "baura@x86_64:~$ lspci", 0xFFF4F2FF);
        ty += 20;
        draw_inter_string_14(fb, x + 16, ty, "00:00.0 Host bridge: Intel 82441FX", 0xFF9CA3AF);
        ty += 18;
        draw_inter_string_14(fb, x + 16, ty, "00:02.0 VGA controller: Bochs VBE", 0xFF9CA3AF);
        ty += 18;
        draw_inter_string_14(fb, x + 16, ty, "00:03.0 Ethernet: Intel 82540EM", 0xFF9CA3AF);
        ty += 24;
        draw_inter_string_14(fb, x + 16, ty, "baura@x86_64:~$ ", 0xFF10B981);
        /* Pulsing cursor */
        draw_rrect_fast(fb, x + 16 + 130, ty - 13, 8, 16, 2, 0xFF9D77FF);
    }
}

void baura_desktop_render_full(const BauraBootFramebuffer *fb, BauraDesktopManager *dm) {
    if (fb == NULL || fb->pixels == NULL) return;
    const int width = (int)fb->width;
    const int height = (int)fb->height;

    /* 1. Wallpaper Dégradé Indigo Nuit vers Violet Profond */
    baura_boot_gradient_rect(fb, 0, 0, width, height, 0xFF0C0924, 0xFF1A1346);

    /* Halos doux en arrière-plan */
    const int cx1 = width / 4, cy1 = height / 3, r1 = 180;
    for (int y = cy1 - r1; y < cy1 + r1; y += 4) {
        if (y < 0 || y >= height) continue;
        const int dy = y - cy1;
        for (int x = cx1 - r1; x < cx1 + r1; x += 4) {
            if (x < 0 || x >= width) continue;
            const int dx = x - cx1;
            if (dx * dx + dy * dy <= r1 * r1) {
                baura_boot_draw_pixel(fb, x, y, 0xFF352B6D);
            }
        }
    }

    /* 2. Top Tray en haut à droite (34px hauteur) */
    draw_rrect_fast(fb, width - 214, 12, 202, 34, 16, 0xFF1A153E);
    draw_inter_string_14(fb, width - 200, 34, "41 \xc2\xb0\x43", 0xFFB4B0DA);
    baura_boot_fill_rect(fb, width - 150, 20, 2, 18, 0xFF352B6D);
    draw_inter_string_14(fb, width - 138, 34, "Dim 8 nov  12:45", 0xFFF4F2FF);

    /* 3. Fenêtres selon le Z-Order (inactive d'abord, active ensuite) */
    for (int i = 0; i < dm->window_count; ++i) {
        if (dm->windows[i].id != dm->focused_window_id) {
            draw_window_frame(fb, &dm->windows[i]);
        }
    }
    for (int i = 0; i < dm->window_count; ++i) {
        if (dm->windows[i].id == dm->focused_window_id) {
            draw_window_frame(fb, &dm->windows[i]);
        }
    }

    /* 4. Split-Dock en bas (Capsule gauche: Démarrer + Apps / Capsule droite: Statut) */
    const int dock_y = height - 58;
    /* Capsule Gauche */
    draw_rrect_fast(fb, 16, dock_y, 420, 48, 18, 0xFF161234);
    /* Bouton Démarrer avec logo vectoriel BAURA */
    if (dm->bstart_open) {
        draw_rrect_fast(fb, 22, dock_y + 4, 40, 40, 12, 0x306D66F2);
    }
    baura_boot_draw_mark(fb, 28, dock_y + 8, 26);
    baura_boot_fill_rect(fb, 64, dock_y + 12, 1, 24, 0xFF322860);

    /* Icônes d'applications du Dock */
    const uint32_t app_cols[6] = {0xFF4F46E5, 0xFF8B5CF6, 0xFF10B981, 0xFFF59E0B, 0xFFEC4899, 0xFF6366F1};
    for (int i = 0; i < 6; ++i) {
        const int ax = 78 + i * 54;
        draw_rrect_fast(fb, ax, dock_y + 7, 34, 34, 10, app_cols[i]);
        /* Indicateur de fenêtre ouverte / active */
        if (i == 0 && dm->windows[0].visible) {
            draw_rrect_fast(fb, ax + 9, dock_y + 43, (dm->focused_window_id == 0) ? 16 : 6, 3, 1,
                            (dm->focused_window_id == 0) ? 0xFF6D66F2 : 0xFF9CA3AF);
        } else if (i == 2 && dm->windows[1].visible) {
            draw_rrect_fast(fb, ax + 9, dock_y + 43, (dm->focused_window_id == 1) ? 16 : 6, 3, 1,
                            (dm->focused_window_id == 1) ? 0xFF10B981 : 0xFF9CA3AF);
        }
    }

    /* Capsule Droite (Statut) */
    draw_rrect_fast(fb, width - 260, dock_y, 244, 48, 18, 0xFF161234);
    draw_inter_string_14(fb, width - 242, dock_y + 30, "Wifi  100%  FR  12:45", 0xFFF4F2FF);

    /* 5. Menu Démarrer Flottant (BStart) si ouvert */
    if (dm->bstart_open) {
        const int mx = 16;
        const int my = dock_y - 320;
        draw_rrect_fast(fb, mx, my, 320, 310, 18, 0xFF18133E);
        /* Barre de recherche Démarrer */
        draw_rrect_fast(fb, mx + 16, my + 16, 288, 36, 12, 0xFF241C54);
        draw_inter_string_14(fb, mx + 28, my + 38, "Rechercher une application...", 0xFF8E8AA8);
        /* Grille des applications */
        draw_inter_string_14(fb, mx + 20, my + 78, "APPLICATIONS \xc3\x89PINGL\xc3\x89\x45\x53", 0xFF6D66F2);
        for (int row = 0; row < 2; ++row) {
            for (int col = 0; col < 4; ++col) {
                const int px = mx + 20 + col * 72;
                const int py = my + 94 + row * 68;
                draw_rrect_fast(fb, px, py, 44, 44, 12, app_cols[(row * 4 + col) % 6]);
            }
        }
        /* Footer Démarrer avec utilisateur */
        baura_boot_fill_rect(fb, mx, my + 258, 320, 1, 0xFF2B225C);
        draw_rrect_fast(fb, mx + 16, my + 268, 30, 30, 15, 0xFF6D66F2);
        draw_inter_string_14(fb, mx + 56, my + 288, "Ahmed Silu\xc3\xa9 \xc2\xb7 Admin", 0xFFF4F2FF);
    }

    /* 6. Curseur de souris au-dessus de tout */
    cursor_draw(fb, dm);
}

void baura_desktop_handle_mouse(const BauraBootFramebuffer *fb, BauraDesktopManager *dm, int dx, int dy, uint8_t buttons) {
    if (dm == NULL || fb == NULL || fb->pixels == NULL) return;

    cursor_restore(fb, dm);

    /* Update mouse coordinates */
    dm->mouse_x += dx;
    dm->mouse_y += dy;
    if (dm->mouse_x < 0) dm->mouse_x = 0;
    if (dm->mouse_x >= (int)fb->width) dm->mouse_x = (int)fb->width - 1;
    if (dm->mouse_y < 0) dm->mouse_y = 0;
    if (dm->mouse_y >= (int)fb->height) dm->mouse_y = (int)fb->height - 1;

    const uint8_t left_down = buttons & 0x01;
    const uint8_t prev_left = dm->prev_buttons & 0x01;
    const int mx = dm->mouse_x;
    const int my = dm->mouse_y;

    int needs_full_redraw = 0;

    /* Left Mouse Button Pressed (Click Event) */
    if (left_down && !prev_left) {
        /* 1. Check Window Titlebars & Close Buttons (in focus order) */
        int clicked_win = -1;
        for (int i = dm->window_count - 1; i >= 0; --i) {
            BauraWindowItem *w = &dm->windows[i];
            if (!w->visible) continue;
            if (mx >= w->x && mx <= w->x + w->w && my >= w->y && my <= w->y + w->h) {
                clicked_win = i;
                break;
            }
        }

        if (clicked_win >= 0) {
            BauraWindowItem *w = &dm->windows[clicked_win];
            /* Close button check */
            if (mx >= w->x + 10 && mx <= w->x + 28 && my >= w->y + 8 && my <= w->y + 26) {
                w->visible = 0;
                needs_full_redraw = 1;
            } else if (my <= w->y + 34) {
                /* Titlebar Dragging */
                w->dragging = 1;
                w->drag_dx = mx - w->x;
                w->drag_dy = my - w->y;
                dm->focused_window_id = w->id;
                for (int j = 0; j < dm->window_count; ++j) dm->windows[j].focused = (dm->windows[j].id == w->id);
                needs_full_redraw = 1;
            } else {
                /* Click inside window -> Focus */
                dm->focused_window_id = w->id;
                for (int j = 0; j < dm->window_count; ++j) dm->windows[j].focused = (dm->windows[j].id == w->id);
                needs_full_redraw = 1;
            }
        }

        /* 2. Check Bottom Dock Clicks */
        const int dock_y = (int)fb->height - 58;
        if (my >= dock_y && my <= dock_y + 48) {
            /* Start Button Click */
            if (mx >= 16 && mx <= 64) {
                dm->bstart_open = !dm->bstart_open;
                needs_full_redraw = 1;
            }
            /* Fichiers Dock Icon */
            else if (mx >= 78 && mx <= 78 + 34) {
                dm->windows[0].visible = !dm->windows[0].visible;
                if (dm->windows[0].visible) dm->focused_window_id = 0;
                needs_full_redraw = 1;
            }
            /* Terminal Dock Icon */
            else if (mx >= 78 + 2 * 54 && mx <= 78 + 2 * 54 + 34) {
                dm->windows[1].visible = !dm->windows[1].visible;
                if (dm->windows[1].visible) dm->focused_window_id = 1;
                needs_full_redraw = 1;
            }
        }
    }

    /* Left Mouse Button Released */
    if (!left_down && prev_left) {
        for (int i = 0; i < dm->window_count; ++i) {
            if (dm->windows[i].dragging) {
                dm->windows[i].dragging = 0;
            }
        }
    }

    /* While Dragging and Mouse Moves */
    if (left_down) {
        for (int i = 0; i < dm->window_count; ++i) {
            BauraWindowItem *w = &dm->windows[i];
            if (w->dragging) {
                int nx = mx - w->drag_dx;
                int ny = my - w->drag_dy;
                /* Keep within desktop workspace bounds */
                if (ny < 40) ny = 40;
                if (ny > (int)fb->height - 90) ny = (int)fb->height - 90;
                if (nx < -w->w + 80) nx = -w->w + 80;
                if (nx > (int)fb->width - 80) nx = (int)fb->width - 80;
                if (nx != w->x || ny != w->y) {
                    w->x = nx;
                    w->y = ny;
                    needs_full_redraw = 1;
                }
            }
        }
    }

    dm->prev_buttons = buttons;

    if (needs_full_redraw) {
        baura_desktop_render_full(fb, dm);
    } else {
        cursor_draw(fb, dm);
    }
}
