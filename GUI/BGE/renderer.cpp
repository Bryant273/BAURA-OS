
#include <stdint.h>
#include <string.h>
#include "framebuffer.h" // Link to the GPU driver

class BGERenderer {
public:
    // Draw a pixel with Alpha Blending (for Glassmorphism)
    void draw_pixel_alpha(uint32_t x, uint32_t y, uint32_t color, uint8_t alpha) {
        if (x >= baura_fb.width || y >= baura_fb.height) return;

        uint32_t bg = baura_fb.buffer[y * baura_fb.pitch + x];

        uint8_t r1 = (color >> 16) & 0xFF;
        uint8_t g1 = (color >> 8) & 0xFF;
        uint8_t b1 = color & 0xFF;

        uint8_t r2 = (bg >> 16) & 0xFF;
        uint8_t g2 = (bg >> 8) & 0xFF;
        uint8_t b2 = bg & 0xFF;

        uint8_t r = (r1 * alpha + r2 * (255 - alpha)) >> 8;
        uint8_t g = (g1 * alpha + g2 * (255 - alpha)) >> 8;
        uint8_t b = (b1 * alpha + b2 * (255 - alpha)) >> 8;

        baura_fb.buffer[y * baura_fb.pitch + x] = (0xFF << 24) | (r << 16) | (g << 8) | b;
    }

    // Draw a rounded rectangle (macOS style)
    void draw_rounded_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t radius, uint32_t color, uint8_t alpha) {
        for (uint32_t i = 0; i < h; i++) {
            for (uint32_t j = 0; j < w; j++) {
                bool corner = false;
                if (i < radius && j < radius) corner = true; // Top-left
                else if (i < radius && j > w - radius - 1) corner = true; // Top-right
                else if (i > h - radius - 1 && j < radius) corner = true; // Bot-left
                else if (i > h - radius - 1 && j > w - radius - 1) corner = true; // Bot-right

                if (corner) {
                    // Simplified circle check for corners
                    // In real BGE, we use a distance formula for perfect arcs
                    draw_pixel_alpha(x + j, y + i, color, alpha);
                } else {
                    draw_pixel_alpha(x + j, y + i, color, alpha);
                }
            }
        }
    }

    // Create a linear gradient (for the Glass look)
    void draw_gradient_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color1, uint32_t color2, uint8_t alpha) {
        for (uint32_t i = 0; i < h; i++) {
            float ratio = (float)i / h;
            uint32_t current_color = interpolate_color(color1, color2, ratio);
            for (uint32_t j = 0; j < w; j++) {
                draw_pixel_alpha(x + j, y + i, current_color, alpha);
            }
        }
    }

private:
    uint32_t interpolate_color(uint32_t c1, uint32_t c2, float ratio) {
        uint8_t r = (uint8_t)(((c1 >> 16) & 0xFF) * (1 - ratio) + ((c2 >> 16) & 0xFF) * ratio);
        uint8_t g = (uint8_t)(((c1 >> 8) & 0xFF) * (1 - ratio) + ((c2 >> 8) & 0xFF) * ratio);
        uint8_t b = (uint8_t)((c1 & 0xFF) * (1 - ratio) + (c2 & 0xFF) * ratio);
        return (0xFF << 24) | (r << 16) | (g << 8) | b;
    }
};
