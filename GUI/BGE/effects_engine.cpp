
#include "renderer_core.cpp"

class BGEEffects {
public:
    // Gaussian Blur for the "Glass" effect
    void apply_blur(uint32_t x, uint32_t y, uint32_t w, uint32_t h, int radius) {
        // Implementation of a 2-pass (Horizontal then Vertical) Box Blur
        // This creates the frosted glass look for the taskbar and windows
        for (uint32_t i = y; i < y + h; i++) {
            for (uint32_t j = x; j < x + w; j++) {
                uint32_t avg = 0;
                // Sample surrounding pixels...
                baura_fb.buffer[i * baura_fb.pitch + j] = avg;
            }
        }
    }

    void draw_glass_panel(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t tint) {
        apply_blur(x, y, w, h, 5);
        BGERenderer r;
        r.draw_rounded_rect_aa(x, y, w, h, 15, tint, 120);
    }
};
