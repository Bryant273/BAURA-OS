#include "renderer_core.h"

#include <limits>

bool bge_framebuffer_valid(const BGEFramebuffer &framebuffer) {
    if (framebuffer.pixels == nullptr || framebuffer.width == 0 ||
        framebuffer.height == 0 ||
        framebuffer.width >
            static_cast<uint32_t>(std::numeric_limits<int>::max()) ||
        framebuffer.height >
            static_cast<uint32_t>(std::numeric_limits<int>::max()) ||
        framebuffer.stride_pixels < framebuffer.width) return false;
    if (static_cast<size_t>(framebuffer.height) >
        std::numeric_limits<size_t>::max() / framebuffer.stride_pixels) {
        return false;
    }
    return framebuffer.capacity_pixels >=
           static_cast<size_t>(framebuffer.height) *
               framebuffer.stride_pixels;
}

uint32_t bge_blend_pixel(uint32_t background, uint32_t foreground,
                         uint8_t alpha) {
    const uint32_t inverse = 255u - alpha;
    uint32_t result = 0xFF000000u;
    for (uint32_t shift = 0; shift <= 16; shift += 8) {
        const uint32_t fg = (foreground >> shift) & 0xFFu;
        const uint32_t bg = (background >> shift) & 0xFFu;
        result |= ((fg * alpha + bg * inverse + 127u) / 255u) << shift;
    }
    return result;
}
