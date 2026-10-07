#ifndef BAURA_BGE_RENDERER_H
#define BAURA_BGE_RENDERER_H

#include <cstddef>
#include <cstdint>

struct BGEFramebuffer {
    uint32_t *pixels;
    uint32_t width;
    uint32_t height;
    uint32_t stride_pixels;
    size_t capacity_pixels;
};

struct BGERect {
    int x;
    int y;
    int width;
    int height;
};

class BGERenderer {
public:
    explicit BGERenderer(const BGEFramebuffer &framebuffer);
    bool valid() const;
    const BGEFramebuffer &framebuffer() const;
    uint32_t pixel(int x, int y) const;
    void clear(uint32_t color);
    void draw_pixel(int x, int y, uint32_t color);
    void draw_pixel_alpha(int x, int y, uint32_t color, uint8_t alpha);
    void fill_rect(int x, int y, int width, int height, uint32_t color);
    void draw_gradient_rect(int x, int y, int width, int height,
                            uint32_t color_top, uint32_t color_bottom);
    void draw_rounded_rect(int x, int y, int width, int height, int radius,
                           uint32_t color, uint8_t alpha = 255);
    void draw_line(int x0, int y0, int x1, int y1, uint32_t color);
    void draw_text(int x, int y, const char *text, uint32_t color,
                   unsigned scale = 1);
    void draw_icon(int x, int y, int size, uint32_t color,
                   const char *icon_name);

    static uint32_t blend_pixels(uint32_t background, uint32_t foreground,
                                 uint8_t alpha);

private:
    BGEFramebuffer target;
    bool clip_rect(int &x, int &y, int &width, int &height) const;
};

#endif
