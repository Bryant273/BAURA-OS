#include "baura_boot_splash.h"
#include "baura_inter_font.h"
#include "baura_bge_boot_data.h"

static uint32_t convert_color(uint32_t argb, uint8_t format) {
    if (format == BAURA_BOOT_FB_FORMAT_BGRX) {
        const uint32_t r = (argb >> 16) & 0xFFu;
        const uint32_t g = (argb >> 8) & 0xFFu;
        const uint32_t b = argb & 0xFFu;
        return (b << 16) | (g << 8) | r;
    }
    return argb & 0x00FFFFFFu;
}

static uint32_t isqrt(uint32_t n) {
    uint32_t root = 0;
    uint32_t bit = 1u << 30;
    while (bit > n) bit >>= 2;
    while (bit != 0) {
        if (n >= root + bit) {
            n -= root + bit;
            root = (root >> 1) + bit;
        } else {
            root >>= 1;
        }
        bit >>= 2;
    }
    return root;
}

static inline uint32_t blend_color_native(uint32_t dst_native, uint32_t src_native, uint32_t alpha_256, uint8_t format) {
    uint32_t dr, dg, db, sr, sg, sb;
    if (format == BAURA_BOOT_FB_FORMAT_BGRX) {
        db = dst_native & 0xFFu;
        dg = (dst_native >> 8) & 0xFFu;
        dr = (dst_native >> 16) & 0xFFu;
        sb = src_native & 0xFFu;
        sg = (src_native >> 8) & 0xFFu;
        sr = (src_native >> 16) & 0xFFu;
    } else {
        dr = (dst_native >> 16) & 0xFFu;
        dg = (dst_native >> 8) & 0xFFu;
        db = dst_native & 0xFFu;
        sr = (src_native >> 16) & 0xFFu;
        sg = (src_native >> 8) & 0xFFu;
        sb = src_native & 0xFFu;
    }

    const uint32_t inv_a = 256u - alpha_256;
    const uint32_t out_r = (dr * inv_a + sr * alpha_256) >> 8;
    const uint32_t out_g = (dg * inv_a + sg * alpha_256) >> 8;
    const uint32_t out_b = (db * inv_a + sb * alpha_256) >> 8;

    return (out_r << 16) | (out_g << 8) | out_b;
}

int baura_boot_framebuffer_validate(const BauraBootFramebuffer *fb) {
    if (fb == 0 || fb->pixels == 0) return BAURA_BOOT_SPLASH_ERR_ARGUMENT;
    if (fb->width < 320u || fb->width > 7680u) return BAURA_BOOT_SPLASH_ERR_ARGUMENT;
    if (fb->height < 240u || fb->height > 4320u) return BAURA_BOOT_SPLASH_ERR_ARGUMENT;
    if (fb->bpp != 32u) return BAURA_BOOT_SPLASH_ERR_UNSUPPORTED;
    if (fb->pitch < fb->width * 4u) return BAURA_BOOT_SPLASH_ERR_ARGUMENT;
    return BAURA_BOOT_SPLASH_OK;
}

void baura_boot_draw_pixel(const BauraBootFramebuffer *fb, int x, int y, uint32_t argb) {
    if (fb == 0 || fb->pixels == 0) return;
    if (x < 0 || (uint32_t)x >= fb->width || y < 0 || (uint32_t)y >= fb->height) return;
    volatile uint32_t *line = (volatile uint32_t *)((uint8_t *)fb->pixels + (size_t)y * fb->pitch);
    line[x] = convert_color(argb, fb->format);
}

void baura_boot_fill_rect(const BauraBootFramebuffer *fb, int x, int y, int w, int h, uint32_t color) {
    if (fb == 0 || fb->pixels == 0 || w <= 0 || h <= 0) return;
    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + w;
    int y1 = y + h;
    if ((uint32_t)x1 > fb->width) x1 = (int)fb->width;
    if ((uint32_t)y1 > fb->height) y1 = (int)fb->height;
    if (x0 >= x1 || y0 >= y1) return;

    const uint32_t native_color = convert_color(color, fb->format);
    for (int py = y0; py < y1; ++py) {
        volatile uint32_t *line = (volatile uint32_t *)((uint8_t *)fb->pixels + (size_t)py * fb->pitch);
        for (int px = x0; px < x1; ++px) {
            line[px] = native_color;
        }
    }
}

void baura_boot_gradient_rect(const BauraBootFramebuffer *fb, int x, int y, int w, int h,
                              uint32_t top_color, uint32_t bottom_color) {
    if (fb == 0 || fb->pixels == 0 || w <= 0 || h <= 0) return;
    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + w;
    int y1 = y + h;
    if ((uint32_t)x1 > fb->width) x1 = (int)fb->width;
    if ((uint32_t)y1 > fb->height) y1 = (int)fb->height;
    if (x0 >= x1 || y0 >= y1) return;

    const int r0 = (int)((top_color >> 16) & 0xFFu);
    const int g0 = (int)((top_color >> 8) & 0xFFu);
    const int b0 = (int)(top_color & 0xFFu);
    const int r1 = (int)((bottom_color >> 16) & 0xFFu);
    const int g1 = (int)((bottom_color >> 8) & 0xFFu);
    const int b1 = (int)(bottom_color & 0xFFu);

    for (int py = y0; py < y1; ++py) {
        const int progress = (h > 1) ? ((py - y) * 256 / (h - 1)) : 0;
        const uint32_t r = (uint32_t)(r0 + ((r1 - r0) * progress) / 256);
        const uint32_t g = (uint32_t)(g0 + ((g1 - g0) * progress) / 256);
        const uint32_t b = (uint32_t)(b0 + ((b1 - b0) * progress) / 256);
        const uint32_t color = (r << 16) | (g << 8) | b;
        const uint32_t native_color = convert_color(color, fb->format);

        volatile uint32_t *line = (volatile uint32_t *)((uint8_t *)fb->pixels + (size_t)py * fb->pitch);
        for (int px = x0; px < x1; ++px) {
            line[px] = native_color;
        }
    }
}

static void draw_rrect(const BauraBootFramebuffer *fb, int x0, int y0, int w, int h, int rad, uint32_t color) {
    if (fb == 0 || fb->pixels == 0 || w <= 0 || h <= 0) return;
    int x1 = x0 + w;
    int y1 = y0 + h;
    int clip_x0 = x0 < 0 ? 0 : x0;
    int clip_y0 = y0 < 0 ? 0 : y0;
    int clip_x1 = x1 > (int)fb->width ? (int)fb->width : x1;
    int clip_y1 = y1 > (int)fb->height ? (int)fb->height : y1;
    if (clip_x0 >= clip_x1 || clip_y0 >= clip_y1) return;

    const uint32_t native_color = convert_color(color, fb->format);
    const int rad2 = rad * rad;

    for (int y = clip_y0; y < clip_y1; ++y) {
        volatile uint32_t *line = (volatile uint32_t *)((uint8_t *)fb->pixels + (size_t)y * fb->pitch);
        for (int x = clip_x0; x < clip_x1; ++x) {
            int dx = 0;
            if (x < x0 + rad) dx = x0 + rad - x;
            else if (x > x1 - rad - 1) dx = x - (x1 - rad - 1);

            int dy = 0;
            if (y < y0 + rad) dy = y0 + rad - y;
            else if (y > y1 - rad - 1) dy = y - (y1 - rad - 1);

            if (dx * dx + dy * dy <= rad2) {
                line[x] = native_color;
            }
        }
    }
}

/* Antialiased Text Drawing using baked Inter Font */
static void draw_inter_char_28(const BauraBootFramebuffer *fb, int x, int baseline, char c, uint32_t color) {
    if (c < 32 || c > 126) return;
    const InterGlyph_28_semibold *g = &inter_glyphs_28_semibold[c - 32];
    if (g->w == 0 || g->h == 0 || g->bitmap == 0) return;

    const int gx = x + g->bx;
    const int gy = baseline + g->by;
    const uint32_t native_c = convert_color(color, fb->format);

    for (int dy = 0; dy < g->h; ++dy) {
        const int py = gy + dy;
        if (py < 0 || (uint32_t)py >= fb->height) continue;
        volatile uint32_t *line = (volatile uint32_t *)((uint8_t *)fb->pixels + (size_t)py * fb->pitch);
        for (int dx = 0; dx < g->w; ++dx) {
            const int px = gx + dx;
            if (px < 0 || (uint32_t)px >= fb->width) continue;
            const uint8_t a = g->bitmap[dy * g->w + dx];
            if (a > 0) {
                line[px] = blend_color_native(line[px], native_c, a, fb->format);
            }
        }
    }
}

int measure_inter_string_28(const char *str) {
    if (str == 0) return 0;
    int w = 0;
    for (size_t i = 0; str[i] != '\0'; ++i) {
        char c = str[i];
        if (c >= 32 && c <= 126) {
            w += inter_glyphs_28_semibold[c - 32].adv;
        } else {
            w += 14;
        }
    }
    return w;
}

void draw_inter_string_28(const BauraBootFramebuffer *fb, int x, int baseline, const char *str, uint32_t color) {
    if (fb == 0 || str == 0) return;
    int pen_x = x;
    for (size_t i = 0; str[i] != '\0'; ++i) {
        char c = str[i];
        if (c >= 32 && c <= 126) {
            draw_inter_char_28(fb, pen_x, baseline, c, color);
            pen_x += inter_glyphs_28_semibold[c - 32].adv;
        } else {
            pen_x += 14;
        }
    }
}

static void draw_inter_char_14(const BauraBootFramebuffer *fb, int x, int baseline, char c, uint32_t color) {
    if (c < 32 || c > 126) return;
    const InterGlyph_14_regular *g = &inter_glyphs_14_regular[c - 32];
    if (g->w == 0 || g->h == 0 || g->bitmap == 0) return;

    const int gx = x + g->bx;
    const int gy = baseline + g->by;
    const uint32_t native_c = convert_color(color, fb->format);

    for (int dy = 0; dy < g->h; ++dy) {
        const int py = gy + dy;
        if (py < 0 || (uint32_t)py >= fb->height) continue;
        volatile uint32_t *line = (volatile uint32_t *)((uint8_t *)fb->pixels + (size_t)py * fb->pitch);
        for (int dx = 0; dx < g->w; ++dx) {
            const int px = gx + dx;
            if (px < 0 || (uint32_t)px >= fb->width) continue;
            const uint8_t a = g->bitmap[dy * g->w + dx];
            if (a > 0) {
                line[px] = blend_color_native(line[px], native_c, a, fb->format);
            }
        }
    }
}

int measure_inter_string_14(const char *str) {
    if (str == 0) return 0;
    int w = 0;
    for (size_t i = 0; str[i] != '\0'; ++i) {
        char c = str[i];
        if (c >= 32 && c <= 126) {
            w += inter_glyphs_14_regular[c - 32].adv;
        } else {
            w += 8;
        }
    }
    return w;
}

void draw_inter_string_14(const BauraBootFramebuffer *fb, int x, int baseline, const char *str, uint32_t color) {
    if (fb == 0 || str == 0) return;
    int pen_x = x;
    for (size_t i = 0; str[i] != '\0'; ++i) {
        char c = str[i];
        if (c >= 32 && c <= 126) {
            draw_inter_char_14(fb, pen_x, baseline, c, color);
            pen_x += inter_glyphs_14_regular[c - 32].adv;
        } else {
            pen_x += 8;
        }
    }
}

void baura_boot_draw_char(const BauraBootFramebuffer *fb, int x, int y, char c, uint32_t color, int scale) {
    (void)scale;
    draw_inter_char_14(fb, x, y + 14, c, color);
}

void baura_boot_draw_string(const BauraBootFramebuffer *fb, int x, int y, const char *str, uint32_t color, int scale) {
    if (scale >= 2) {
        draw_inter_string_28(fb, x, y + 28, str, color);
    } else {
        draw_inter_string_14(fb, x, y + 14, str, color);
    }
}

void baura_boot_draw_mark(const BauraBootFramebuffer *fb, int x, int y, int size) {
    if (fb == 0 || size < 16) return;

    /* Symbol viewBox: 150 29 199 420 (width 199, height 420) */
    const int S_denom = 420;

    /* 1. b1 - Barre verticale principale (Indigo #4F46E5) */
    const int b1_w = (48 * size) / S_denom > 3 ? (48 * size) / S_denom : 3;
    const int b1_rx = (24 * size) / S_denom > 2 ? (24 * size) / S_denom : 2;
    draw_rrect(fb, x, y, b1_w, size, b1_rx, BAURA_BOOT_COLOR_B1_INDIGO);

    /* 2. Demi-disques supérieur et inférieur */
    const int petal_base_x = x + (98 * size) / S_denom;

    /* b2 - Demi-disque supérieur (Violet #8B5CF6) */
    const int top_y_start = y;
    const int top_h = (205 * size) / S_denom;
    const int ry_top = top_h / 2 > 1 ? top_h / 2 : 1;
    const int rx_top = (101 * size) / S_denom;

    for (int dy = 0; dy < top_h; ++dy) {
        const int dist_y = dy - ry_top;
        if (dist_y >= -ry_top && dist_y <= ry_top) {
            const int term = (int)(((int64_t)dist_y * dist_y * 1024) / (ry_top * ry_top + 1));
            if (term <= 1024) {
                const uint32_t factor = isqrt((uint32_t)(1024 - term));
                const int span_w = (int)((rx_top * factor) >> 5);
                if (span_w > 0) {
                    baura_boot_fill_rect(fb, petal_base_x, top_y_start + dy, span_w, 1, BAURA_BOOT_COLOR_B2_VIOLET);
                }
            }
        }
    }

    /* b3 - Demi-disque inférieur (Lavande #C4B5FD) */
    const int bot_y_start = y + (216 * size) / S_denom;
    const int bot_h = (204 * size) / S_denom;
    const int ry_bot = bot_h / 2 > 1 ? bot_h / 2 : 1;
    const int rx_bot = (101 * size) / S_denom;

    for (int dy = 0; dy < bot_h; ++dy) {
        const int dist_y = dy - ry_bot;
        if (dist_y >= -ry_bot && dist_y <= ry_bot) {
            const int term = (int)(((int64_t)dist_y * dist_y * 1024) / (ry_bot * ry_bot + 1));
            if (term <= 1024) {
                const uint32_t factor = isqrt((uint32_t)(1024 - term));
                const int span_w = (int)((rx_bot * factor) >> 5);
                if (span_w > 0) {
                    baura_boot_fill_rect(fb, petal_base_x, bot_y_start + dy, span_w, 1, BAURA_BOOT_COLOR_B3_LAVANDE);
                }
            }
        }
    }

    /* 4. b4 - Point doré signature (Or #D0AE2A) */
    const int dot_x = x + (166 * size) / S_denom;
    const int dot_y = y + (192 * size) / S_denom;
    const int dot_w = (33 * size) / S_denom > 4 ? (33 * size) / S_denom : 4;
    const int dot_h = (36 * size) / S_denom > 4 ? (36 * size) / S_denom : 4;
    const int dot_rx = (15 * size) / S_denom > 2 ? (15 * size) / S_denom : 2;
    draw_rrect(fb, dot_x, dot_y, dot_w, dot_h, dot_rx, BAURA_BOOT_COLOR_B4_OR);
}

int baura_boot_draw_splash(const BauraBootFramebuffer *fb, const char *status, uint32_t progress_pct) {
    const int valid = baura_boot_framebuffer_validate(fb);
    if (valid != BAURA_BOOT_SPLASH_OK) return valid;

    const int width = (int)fb->width;
    const int height = (int)fb->height;
    const int cx = width / 2;
    const int cy = (height * 44) / 100;

    /* 1. Linear background gradient (Indigo Night -> Deep Violet #120F3A -> #2E1A7A) */
    baura_boot_gradient_rect(fb, 0, 0, width, height,
                             BAURA_BOOT_COLOR_BG_START, BAURA_BOOT_COLOR_BG_END);

    /* 2. Blit haute-fidélité du splash vectoriel BGE centré (640x360) */
    const int splash_w = BAURA_BGE_SPLASH_WIDTH;
    const int splash_h = BAURA_BGE_SPLASH_HEIGHT;
    const int offset_x = (width - splash_w) / 2;
    const int offset_y = (height - splash_h) / 2;

    if (offset_x >= 0 && offset_y >= 0) {
        int cur_x = 0;
        int cur_y = 0;
        for (size_t i = 0; i < BAURA_BGE_SPLASH_RUNS_COUNT && cur_y < splash_h; ++i) {
            const uint32_t native_c = convert_color(baura_bge_splash_runs[i].color, fb->format);
            const uint16_t len = baura_bge_splash_runs[i].length;
            for (uint16_t k = 0; k < len && cur_y < splash_h; ++k) {
                baura_boot_draw_pixel(fb, offset_x + cur_x, offset_y + cur_y, native_c);
                cur_x++;
                if (cur_x >= splash_w) {
                    cur_x = 0;
                    cur_y++;
                }
            }
        }
    } else {
        /* Fallback vectoriel direct */
        const int emblem_h = (height * 30) / 100;
        const int emblem_w = (199 * emblem_h) / 420;
        baura_boot_draw_mark(fb, cx - emblem_w / 2, cy - emblem_h / 2, emblem_h);
    }

    /* 4. Wordmark "BAURA OS" in Baked Inter SemiBold 28px */
    const char *title = "BAURA OS";
    const int title_w = measure_inter_string_28(title);
    const int title_x = cx - title_w / 2;
    const int title_baseline = (height * 72) / 100;

    draw_inter_string_28(fb, title_x, title_baseline, title, BAURA_BOOT_COLOR_TEXT_PRIMARY);

    /* 5. Slim progress bar (220 x 6, rounded radius 3) */
    const int bar_w = 220;
    const int bar_h = 6;
    const int bar_x = cx - bar_w / 2;
    const int bar_y = title_baseline + 28;

    /* Background pill */
    draw_rrect(fb, bar_x, bar_y, bar_w, bar_h, 3, BAURA_BOOT_COLOR_BAR_BG);

    /* Progress fill with gradient from Indigo to Violet */
    uint32_t pct = progress_pct;
    if (pct > 100u) pct = 100u;
    int filled_w = (int)((uint64_t)bar_w * pct / 100u);
    if (filled_w < 6 && pct > 0) filled_w = 6;

    if (filled_w > 0) {
        for (int bx = 0; bx < filled_w; ++bx) {
            const int progress_local = (bx * 256) / bar_w;
            const uint32_t r = 0x4Fu + ((0x8Bu - 0x4Fu) * (uint32_t)progress_local) / 256u;
            const uint32_t g = 0x46u + ((0x5Cu - 0x46u) * (uint32_t)progress_local) / 256u;
            const uint32_t b = 0xE5u + ((0xF6u - 0xE5u) * (uint32_t)progress_local) / 256u;
            const uint32_t grad_color = (r << 16) | (g << 8) | b;
            const uint32_t native_c = convert_color(grad_color, fb->format);

            for (int by = 0; by < bar_h; ++by) {
                int dx = 0;
                if (bx < 3) dx = 3 - bx;
                else if (bx > filled_w - 4) dx = bx - (filled_w - 4);
                int dy = 0;
                if (by < 3) dy = 3 - by;
                else if (by > 2) dy = by - 2;

                if (dx * dx + dy * dy <= 9) {
                    baura_boot_draw_pixel(fb, bar_x + bx, bar_y + by, native_c);
                }
            }
        }
    }

    /* 6. Status message in Baked Inter Regular 14px */
    const char *msg = status != 0 ? status : "INITIALIZING SYSTEM";
    const int status_w = measure_inter_string_14(msg);
    const int status_x = cx - status_w / 2;
    const int status_baseline = bar_y + 30;

    draw_inter_string_14(fb, status_x, status_baseline, msg, BAURA_BOOT_COLOR_TEXT_SECONDARY);

    return BAURA_BOOT_SPLASH_OK;
}

static inline uint32_t interpolate_color(uint32_t c0, uint32_t c1, int y, int total_h) {
    if (y <= 0) return c0;
    if (y >= total_h) return c1;
    const uint32_t r0 = (c0 >> 16) & 0xFFu, g0 = (c0 >> 8) & 0xFFu, b0 = c0 & 0xFFu;
    const uint32_t r1 = (c1 >> 16) & 0xFFu, g1 = (c1 >> 8) & 0xFFu, b1 = c1 & 0xFFu;
    const uint32_t r = r0 + ((r1 - r0) * (uint32_t)y) / (uint32_t)total_h;
    const uint32_t g = g0 + ((g1 - g0) * (uint32_t)y) / (uint32_t)total_h;
    const uint32_t b = b0 + ((b1 - b0) * (uint32_t)y) / (uint32_t)total_h;
    return (r << 16) | (g << 8) | b;
}

void baura_boot_update_progress(const BauraBootFramebuffer *fb, const char *status, uint32_t progress_pct) {
    if (fb == 0 || fb->pixels == 0) return;
    const int width = (int)fb->width;
    const int height = (int)fb->height;
    const int cx = width / 2;
    const int title_baseline = (height * 72) / 100;
    const int bar_w = 220;
    const int bar_h = 6;
    const int bar_x = cx - bar_w / 2;
    const int bar_y = title_baseline + 28;
    const int status_baseline = bar_y + 30;

    const int clear_y0 = bar_y - 4;
    const int clear_h = (status_baseline + 18) - clear_y0;
    baura_boot_gradient_rect(fb, cx - 260, clear_y0, 520, clear_h,
                             interpolate_color(BAURA_BOOT_COLOR_BG_START, BAURA_BOOT_COLOR_BG_END, clear_y0, height),
                             interpolate_color(BAURA_BOOT_COLOR_BG_START, BAURA_BOOT_COLOR_BG_END, clear_y0 + clear_h, height));

    draw_rrect(fb, bar_x, bar_y, bar_w, bar_h, 3, BAURA_BOOT_COLOR_BAR_BG);

    uint32_t pct = progress_pct;
    if (pct > 100u) pct = 100u;
    int filled_w = (int)((uint64_t)bar_w * pct / 100u);
    if (filled_w < 6 && pct > 0) filled_w = 6;

    if (filled_w > 0) {
        for (int bx = 0; bx < filled_w; ++bx) {
            const int progress_local = (bx * 256) / bar_w;
            const uint32_t r = 0x4Fu + ((0x8Bu - 0x4Fu) * (uint32_t)progress_local) / 256u;
            const uint32_t g = 0x46u + ((0x5Cu - 0x46u) * (uint32_t)progress_local) / 256u;
            const uint32_t b = 0xE5u + ((0xF6u - 0xE5u) * (uint32_t)progress_local) / 256u;
            const uint32_t grad_color = (r << 16) | (g << 8) | b;
            const uint32_t native_c = convert_color(grad_color, fb->format);

            for (int by = 0; by < bar_h; ++by) {
                int dx = 0;
                if (bx < 3) dx = 3 - bx;
                else if (bx > filled_w - 4) dx = bx - (filled_w - 4);
                int dy = 0;
                if (by < 3) dy = 3 - by;
                else if (by > 2) dy = by - 2;

                if (dx * dx + dy * dy <= 9) {
                    baura_boot_draw_pixel(fb, bar_x + bx, bar_y + by, native_c);
                }
            }
        }
    }

    const char *msg = status != 0 ? status : "INITIALIZING SYSTEM";
    const int status_w = measure_inter_string_14(msg);
    const int status_x = cx - status_w / 2;
    draw_inter_string_14(fb, status_x, status_baseline, msg, BAURA_BOOT_COLOR_TEXT_SECONDARY);
}

void baura_boot_draw_desktop(const BauraBootFramebuffer *fb, int cursor_visible) {
    (void)cursor_visible;
    if (fb == 0 || fb->pixels == 0) return;
    const int width = (int)fb->width;
    const int height = (int)fb->height;

    /* 1. Blit haute-fidélité direct de la scène BGE complète (1024x768) */
    if ((uint32_t)width == BAURA_BGE_DESKTOP_WIDTH && (uint32_t)height == BAURA_BGE_DESKTOP_HEIGHT) {
        size_t current_x = 0;
        size_t current_y = 0;
        volatile uint32_t *line = (volatile uint32_t *)fb->pixels;
        for (size_t i = 0; i < BAURA_BGE_DESKTOP_RUNS_COUNT && current_y < (size_t)height; ++i) {
            const uint32_t native_c = convert_color(baura_bge_desktop_runs[i].color, fb->format);
            const uint16_t len = baura_bge_desktop_runs[i].length;
            for (uint16_t k = 0; k < len && current_y < (size_t)height; ++k) {
                line[current_x++] = native_c;
                if (current_x >= (size_t)width) {
                    current_x = 0;
                    current_y++;
                    line = (volatile uint32_t *)((uint8_t *)fb->pixels + current_y * fb->pitch);
                }
            }
        }
        return;
    }

    /* Fallback dynamique pour autres résolutions */
    baura_boot_gradient_rect(fb, 0, 0, (int)width, (int)height, 0xFF0C0924, 0xFF1A1346);

    /* Orbes / halos doux en arrière-plan */
    const uint32_t orb_color = convert_color(0xFF4F46E5, fb->format);
    const int cx1 = width / 4, cy1 = height / 3, r1 = 180;
    for (int y = cy1 - r1; y < cy1 + r1; y += 3) {
        if (y < 0 || y >= height) continue;
        const int dy = y - cy1;
        const int dy2 = dy * dy;
        volatile uint32_t *line = (volatile uint32_t *)((uint8_t *)fb->pixels + (size_t)y * fb->pitch);
        for (int x = cx1 - r1; x < cx1 + r1; x += 3) {
            if (x < 0 || x >= width) continue;
            const int dx = x - cx1;
            if (dx * dx + dy2 <= r1 * r1) {
                line[x] = blend_color_native(line[x], orb_color, 8, fb->format);
            }
        }
    }

    /* 2. Top Tray en haut à droite (x=810, y=12, w=200, h=34, rx=16) */
    draw_rrect(fb, width - 214, 12, 202, 34, 16, 0xFF1A153E);
    /* Filet lumineux verre */
    for (int bx = width - 200; bx < width - 26; ++bx) baura_boot_draw_pixel(fb, bx, 12, 0xFF352B6D);
    draw_inter_string_14(fb, width - 202, 35, "41 C", 0xFFB4B0DA);
    baura_boot_fill_rect(fb, width - 150, 20, 2, 18, 0xFF352B6D);
    draw_inter_string_14(fb, width - 138, 35, "Dim 8 nov  12:45", 0xFFF4F2FF);

    /* 3. Fenêtre Arrière-Plan : Gestionnaire de Fichiers (x=40, y=56, w=480, h=380) */
    draw_rrect(fb, 40, 56, 480, 380, 16, 0xFF141032);
    /* Barre de titre Fichiers */
    draw_rrect(fb, 40, 56, 480, 42, 16, 0xFF201A4C);
    baura_boot_fill_rect(fb, 40, 84, 480, 14, 0xFF201A4C); /* comble le bas du rayon */
    draw_rrect(fb, 56, 68, 18, 18, 5, 0xFF4F46E5); /* Icône dossier */
    draw_inter_string_14(fb, 84, 83, "Fichiers  Documents", 0xFFF4F2FF);
    /* Boutons de fenêtre */
    draw_rrect(fb, 485, 71, 12, 12, 6, 0xFFEF4444);
    draw_rrect(fb, 465, 71, 12, 12, 6, 0xFFF59E0B);
    draw_rrect(fb, 445, 71, 12, 12, 6, 0xFF10B981);

    /* Volet latéral gauche de navigation */
    baura_boot_fill_rect(fb, 40, 98, 130, 338, 0xFF18133B);
    baura_boot_fill_rect(fb, 170, 98, 1, 338, 0xFF2B225C); /* séparateur */
    draw_inter_string_14(fb, 52, 126, "> Documents", 0xFF6D66F2);
    draw_inter_string_14(fb, 52, 154, "  Images", 0xFFB4B0DA);
    draw_inter_string_14(fb, 52, 182, "  Musique", 0xFFB4B0DA);
    draw_inter_string_14(fb, 52, 210, "  Disque PC/C:", 0xFFB4B0DA);

    /* Tuiles de dossiers dans la vue Fichiers */
    draw_rrect(fb, 190, 120, 120, 80, 12, 0xFF231B54);
    draw_rrect(fb, 202, 132, 28, 22, 6, 0xFF6D66F2);
    draw_inter_string_14(fb, 202, 185, "Projets", 0xFFF4F2FF);

    draw_rrect(fb, 330, 120, 120, 80, 12, 0xFF231B54);
    draw_rrect(fb, 342, 132, 28, 22, 6, 0xFF8B5CF6);
    draw_inter_string_14(fb, 342, 185, "Factures", 0xFFF4F2FF);

    draw_rrect(fb, 190, 220, 120, 80, 12, 0xFF231B54);
    draw_rrect(fb, 202, 232, 28, 22, 6, 0xFFD0AE2A);
    draw_inter_string_14(fb, 202, 285, "Systeme", 0xFFF4F2FF);

    /* 4. Fenêtre Premier-Plan : Terminal BAURA OS (x=410, y=140, w=570, h=440) */
    draw_rrect(fb, 410, 140, 570, 440, 16, 0xFF0D0F18);
    /* Barre de titre Terminal */
    draw_rrect(fb, 410, 140, 570, 42, 16, 0xFF191D2E);
    baura_boot_fill_rect(fb, 410, 168, 570, 14, 0xFF191D2E);
    /* Boutons de contrôle (rouge, jaune, vert) */
    draw_rrect(fb, 426, 155, 12, 12, 6, 0xFFEF4444);
    draw_rrect(fb, 446, 155, 12, 12, 6, 0xFFF59E0B);
    draw_rrect(fb, 466, 155, 12, 12, 6, 0xFF10B981);
    draw_inter_string_14(fb, 500, 167, "Terminal  baura@x86_64", 0xFFF4F2FF);

    /* Contenu textuel console du terminal */
    int term_y = 210;
    draw_inter_string_14(fb, 430, term_y, "BAURA OS 1.0 (x86_64 pc99) - seL4 16.0.0-dev", 0xFF10B981);
    term_y += 24;
    draw_inter_string_14(fb, 430, term_y, "baura@pc99:~$ uname -a", 0xFFF4F2FF);
    term_y += 22;
    draw_inter_string_14(fb, 430, term_y, "BAURA-OS pc99 1.0.0-seL4 #1 SMP x86_64", 0xFFB4B0DA);
    term_y += 24;
    draw_inter_string_14(fb, 430, term_y, "baura@pc99:~$ lspci", 0xFFF4F2FF);
    term_y += 22;
    draw_inter_string_14(fb, 430, term_y, "00:00.0 Host bridge: Intel 82441FX", 0xFF9CA3AF);
    term_y += 20;
    draw_inter_string_14(fb, 430, term_y, "00:02.0 VGA compatible controller: Bochs VBE", 0xFF9CA3AF);
    term_y += 20;
    draw_inter_string_14(fb, 430, term_y, "00:03.0 Ethernet controller: Intel 82540EM", 0xFF9CA3AF);
    term_y += 24;
    draw_inter_string_14(fb, 430, term_y, "baura@pc99:~$ ls -l PC/C:/", 0xFFF4F2FF);
    term_y += 22;
    draw_inter_string_14(fb, 430, term_y, "drwxr-xr-x Documents/   drwxr-xr-x Projets/", 0xFF6D66F2);
    term_y += 26;
    draw_inter_string_14(fb, 430, term_y, "baura@pc99:~$ ", 0xFF10B981);
    /* Curseur clignotant vert */
    if (cursor_visible) {
        const int cur_x = 430 + measure_inter_string_14("baura@pc99:~$ ");
        baura_boot_fill_rect(fb, cur_x, term_y - 13, 8, 16, 0xFF10B981);
    }

    /* 5. Split-Dock en bas (y=704, h=52, rx=20) */
    /* Capsule gauche (Dock applicatif) */
    draw_rrect(fb, 16, height - 60, 420, 52, 20, 0xFF161234);
    for (int bx = 28; bx < 424; ++bx) baura_boot_draw_pixel(fb, bx, height - 60, 0xFF352B6D);

    /* Bouton Démarrer avec logo vectoriel BAURA 26px */
    baura_boot_draw_mark(fb, 28, height - 48, 26);
    baura_boot_fill_rect(fb, 62, height - 44, 1, 20, 0xFF322860);

    /* 6 Icônes d'applications du Split-Dock */
    const char *app_names[6] = {"Fichiers", "HOUB", "Terminal", "BMedia", "Store", "Reglages"};
    const uint32_t app_colors[6] = {0xFF4F46E5, 0xFF8B5CF6, 0xFF10B981, 0xFFF59E0B, 0xFFEC4899, 0xFF6366F1};
    int app_x = 76;
    for (int i = 0; i < 6; ++i) {
        draw_rrect(fb, app_x, height - 51, 34, 34, 10, app_colors[i]);
        /* Indicateur actif sous le Terminal (index 2) */
        if (i == 2) {
            draw_rrect(fb, app_x + 9, height - 12, 16, 3, 2, 0xFF10B981);
        }
        (void)app_names[i];
        app_x += 54;
    }

    /* Capsule droite (Statut & Accès rapide) */
    draw_rrect(fb, width - 260, height - 60, 244, 52, 20, 0xFF161234);
    for (int bx = width - 248; bx < width - 28; ++bx) baura_boot_draw_pixel(fb, bx, height - 60, 0xFF352B6D);
    draw_inter_string_14(fb, width - 242, height - 28, "Wifi  100%  12:45", 0xFFF4F2FF);
}
