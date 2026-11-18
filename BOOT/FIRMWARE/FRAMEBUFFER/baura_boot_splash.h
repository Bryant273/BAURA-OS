#ifndef BAURA_BOOT_SPLASH_H
#define BAURA_BOOT_SPLASH_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BAURA_BOOT_COLOR_BG_START        UINT32_C(0xFF120F3A) /* nuit indigo charte */
#define BAURA_BOOT_COLOR_BG_END          UINT32_C(0xFF2E1A7A) /* violet sombre profond charte */
#define BAURA_BOOT_COLOR_PANEL_BG        UINT32_C(0xFF1C1750) /* verre sombre */
#define BAURA_BOOT_COLOR_PANEL_BORDER    UINT32_C(0xFF3A3185) /* filet verre */
#define BAURA_BOOT_COLOR_B1_INDIGO       UINT32_C(0xFF4F46E5) /* pilule verticale */
#define BAURA_BOOT_COLOR_B2_VIOLET       UINT32_C(0xFF8B5CF6) /* demi-disque haut */
#define BAURA_BOOT_COLOR_B3_LAVANDE      UINT32_C(0xFFC4B5FD) /* demi-disque bas */
#define BAURA_BOOT_COLOR_B4_OR           UINT32_C(0xFFD0AE2A) /* point dore */
#define BAURA_BOOT_COLOR_TEXT_PRIMARY    UINT32_C(0xFFF4F2FF)
#define BAURA_BOOT_COLOR_TEXT_SECONDARY  UINT32_C(0xFFB4B0DA)
#define BAURA_BOOT_COLOR_BAR_BG          UINT32_C(0xFF2A2368)
#define BAURA_BOOT_COLOR_WHITE           UINT32_C(0xFFFFFFFF)

/* Aliases de retro-compatibilite */
#define BAURA_BOOT_COLOR_ACCENT_BLUE     BAURA_BOOT_COLOR_B1_INDIGO
#define BAURA_BOOT_COLOR_ACCENT_CYAN     BAURA_BOOT_COLOR_B3_LAVANDE

enum {
    BAURA_BOOT_FB_FORMAT_RGBX = 0,
    BAURA_BOOT_FB_FORMAT_BGRX = 1
};

typedef struct {
    void     *pixels;
    uint64_t physical_address;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint8_t  bpp;
    uint8_t  format;
} BauraBootFramebuffer;

enum {
    BAURA_BOOT_SPLASH_OK = 0,
    BAURA_BOOT_SPLASH_ERR_ARGUMENT = -1,
    BAURA_BOOT_SPLASH_ERR_UNSUPPORTED = -2
};

int baura_boot_framebuffer_validate(const BauraBootFramebuffer *fb);
void baura_boot_draw_pixel(const BauraBootFramebuffer *fb, int x, int y, uint32_t argb);
void baura_boot_fill_rect(const BauraBootFramebuffer *fb, int x, int y, int w, int h, uint32_t color);
void baura_boot_gradient_rect(const BauraBootFramebuffer *fb, int x, int y, int w, int h,
                              uint32_t top_color, uint32_t bottom_color);
void baura_boot_draw_char(const BauraBootFramebuffer *fb, int x, int y, char c, uint32_t color, int scale);
void baura_boot_draw_string(const BauraBootFramebuffer *fb, int x, int y, const char *str, uint32_t color, int scale);
void baura_boot_draw_mark(const BauraBootFramebuffer *fb, int x, int y, int size);
void draw_inter_string_14(const BauraBootFramebuffer *fb, int x, int baseline, const char *str, uint32_t color);
int measure_inter_string_14(const char *str);
void draw_inter_string_28(const BauraBootFramebuffer *fb, int x, int baseline, const char *str, uint32_t color);
int measure_inter_string_28(const char *str);
int baura_boot_draw_splash(const BauraBootFramebuffer *fb, const char *status, uint32_t progress_pct);
void baura_boot_update_progress(const BauraBootFramebuffer *fb, const char *status, uint32_t progress_pct);
void baura_boot_draw_desktop(const BauraBootFramebuffer *fb, int cursor_visible);

#ifdef __cplusplus
}
#endif

#endif /* BAURA_BOOT_SPLASH_H */
