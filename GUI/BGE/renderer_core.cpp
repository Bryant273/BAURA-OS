
#include <stdint.h>
#include <cmath>
#include "framebuffer.h"

class BGERenderer {
public:
    // Advanced Alpha Blending (Pre-multiplied alpha)
    static inline uint32_t blend_pixels(uint32_t bg, uint32_t fg, uint8_t alpha) {
        if (alpha == 0) return bg;
        if (alpha == 255) return fg;

        uint32_t rb = (bg & 0xFF00FF) + (((fg & 0xFF00FF) - (bg & 0xFF00FF)) * alpha >> 8);
        uint32_t g = (bg & 0x00FF00) + (((fg & 0x00FF00) - (bg & 0x00FF00)) * alpha >> 8);
        return (rb & 0xFF00FF) | (g & 0x00FF00);
    }

    // High-quality rounded rectangle with anti-aliasing
    void draw_rounded_rect_aa(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t r, uint32_t color, uint8_t alpha) {
        for (uint32_t i = 0; i < h; i++) {
            for (uint32_t j = 0; j < w; j++) {
                float dx = (j < r) ? (j - r) : (j > w - r - 1) ? (j - (w - r - 1)) : 0;
                float dy = (i < r) ? (i - r) : (i > h - r - 1) ? (i - (h - r - 1)) : 0;
                float dist = std::sqrt(dx*dx + dy*dy);

                if (dist <= r) {
                    uint8_t final_alpha = alpha;
                    if (dist > r - 1.0f) final_alpha = (uint8_t)((r - dist) * 255);
                    draw_pixel_alpha(x + j, y + i, color, final_alpha);
                }
            }
        }
    }

    void draw_pixel_alpha(uint32_t x, uint32_t y, uint32_t color, uint8_t alpha) {
        if (x >= baura_fb.width || y >= baura_fb.height) return;
        uint32_t bg = baura_fb.buffer[y * baura_fb.pitch + x];
        baura_fb.buffer[y * baura_//pitch + x] = blend_pixels(bg, color, alpha);
    }
};
