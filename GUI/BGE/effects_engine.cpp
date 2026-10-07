#include "effects_engine.h"

#include <algorithm>
#include <new>
#include <vector>

BGEEffects::BGEEffects(BGERenderer &target) : renderer(target) {}

bool BGEEffects::apply_blur(int x, int y, int width, int height, int radius) {
    if (!renderer.valid() || width <= 0 || height <= 0 || radius < 0) {
        return false;
    }
    const BGEFramebuffer &fb = renderer.framebuffer();
    const int left = std::max(0, x);
    const int top = std::max(0, y);
    const int right = static_cast<int>(std::min<int64_t>(
        fb.width, static_cast<int64_t>(x) + width));
    const int bottom = static_cast<int>(std::min<int64_t>(
        fb.height, static_cast<int64_t>(y) + height));
    if (right <= left || bottom <= top) return true;
    const size_t region_width = static_cast<size_t>(right - left);
    const size_t region_height = static_cast<size_t>(bottom - top);
    if (region_width > 4194304u / region_height) return false;
    const size_t pixel_count = region_width * region_height;
    std::vector<uint32_t> source;
    std::vector<uint32_t> horizontal;
    try {
        source.resize(pixel_count);
        horizontal.resize(pixel_count);
    } catch (const std::bad_alloc &) {
        return false;
    }
    for (int row = top; row < bottom; ++row) {
        for (int column = left; column < right; ++column) {
            const size_t index = static_cast<size_t>(row - top) * region_width +
                                 static_cast<size_t>(column - left);
            source[index] = renderer.pixel(column, row);
        }
    }
    const int bounded_radius = std::min(radius, 32);
    const uint32_t samples = static_cast<uint32_t>(bounded_radius * 2 + 1);
    for (int row = 0; row < bottom - top; ++row) {
        uint32_t sums[3] = {0, 0, 0};
        const auto add_horizontal = [&](int64_t column, int direction) {
            const int sample_x = static_cast<int>(std::max<int64_t>(
                0, std::min<int64_t>(right - left - 1, column)));
            const uint32_t color =
                source[static_cast<size_t>(row) * region_width + sample_x];
            sums[0] = static_cast<uint32_t>(
                static_cast<int64_t>(sums[0]) +
                direction * static_cast<int64_t>((color >> 16) & 0xFFu));
            sums[1] = static_cast<uint32_t>(
                static_cast<int64_t>(sums[1]) +
                direction * static_cast<int64_t>((color >> 8) & 0xFFu));
            sums[2] = static_cast<uint32_t>(
                static_cast<int64_t>(sums[2]) +
                direction * static_cast<int64_t>(color & 0xFFu));
        };
        for (int offset = -bounded_radius; offset <= bounded_radius; ++offset) {
            add_horizontal(offset, 1);
        }
        for (int column = 0; column < right - left; ++column) {
            horizontal[static_cast<size_t>(row) * region_width + column] =
                0xFF000000u | ((sums[0] / samples) << 16) |
                ((sums[1] / samples) << 8) | (sums[2] / samples);
            add_horizontal(static_cast<int64_t>(column) - bounded_radius, -1);
            add_horizontal(static_cast<int64_t>(column) + bounded_radius + 1,
                           1);
        }
    }
    for (int column = 0; column < right - left; ++column) {
        uint32_t sums[3] = {0, 0, 0};
        const auto add_vertical = [&](int64_t source_row, int direction) {
            const int sample_y = static_cast<int>(std::max<int64_t>(
                0, std::min<int64_t>(bottom - top - 1, source_row)));
            const uint32_t color = horizontal[
                static_cast<size_t>(sample_y) * region_width + column];
            sums[0] = static_cast<uint32_t>(
                static_cast<int64_t>(sums[0]) +
                direction * static_cast<int64_t>((color >> 16) & 0xFFu));
            sums[1] = static_cast<uint32_t>(
                static_cast<int64_t>(sums[1]) +
                direction * static_cast<int64_t>((color >> 8) & 0xFFu));
            sums[2] = static_cast<uint32_t>(
                static_cast<int64_t>(sums[2]) +
                direction * static_cast<int64_t>(color & 0xFFu));
        };
        for (int offset = -bounded_radius; offset <= bounded_radius;
             ++offset) {
            add_vertical(offset, 1);
        }
        for (int current_row = 0; current_row < bottom - top;
             ++current_row) {
            const uint32_t blurred = 0xFF000000u |
                ((sums[0] / samples) << 16) |
                ((sums[1] / samples) << 8) | (sums[2] / samples);
            renderer.draw_pixel(left + column, top + current_row, blurred);
            add_vertical(static_cast<int64_t>(current_row) - bounded_radius,
                         -1);
            add_vertical(static_cast<int64_t>(current_row) + bounded_radius +
                             1,
                         1);
        }
    }
    return true;
}

bool BGEEffects::draw_glass_panel(int x, int y, int width, int height,
                                  int radius, uint32_t tint, uint8_t alpha) {
    if (!apply_blur(x, y, width, height, 5)) return false;
    renderer.draw_rounded_rect(x, y, width, height, radius, tint, alpha);
    renderer.draw_rounded_rect(x, y, width, height, radius, 0xFFFFFFFFu, 28);
    return true;
}
