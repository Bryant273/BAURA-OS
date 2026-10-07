#include "renderer.h"
#include "renderer_core.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

static uint8_t glyph_row(char character, unsigned row) {
    static const uint8_t digits[10][7] = {
        {14,17,19,21,25,17,14}, {4,12,4,4,4,4,14},
        {14,17,1,2,4,8,31}, {30,1,1,14,1,1,30},
        {2,6,10,18,31,2,2}, {31,16,16,30,1,1,30},
        {14,16,16,30,17,17,14}, {31,1,2,4,8,8,8},
        {14,17,17,14,17,17,14}, {14,17,17,15,1,1,14}
    };
    static const uint8_t letters[26][7] = {
        {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},
        {14,17,16,16,16,17,14},{30,17,17,17,17,17,30},
        {31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
        {14,17,16,23,17,17,15},{17,17,17,31,17,17,17},
        {14,4,4,4,4,4,14},{7,2,2,2,18,18,12},
        {17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
        {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},
        {14,17,17,17,17,17,14},{30,17,17,30,16,16,16},
        {14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
        {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},
        {17,17,17,17,17,17,14},{17,17,17,17,17,10,4},
        {17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
        {17,17,10,4,4,4,4},{31,1,2,4,8,16,31}
    };
    if (row >= 7) return 0;
    if (character >= '0' && character <= '9') return digits[character - '0'][row];
    if (character >= 'a' && character <= 'z') character -= 'a' - 'A';
    if (character >= 'A' && character <= 'Z') return letters[character - 'A'][row];
    switch (character) {
    case '-': return row == 3 ? 14 : 0;
    case '+': {
        static const uint8_t plus[7] = {0,4,4,31,4,4,0};
        return plus[row];
    }
    case '%': {
        static const uint8_t percent[7] = {17,2,4,8,17,0,0};
        return percent[row];
    }
    case '*': {
        static const uint8_t star[7] = {0,21,14,31,14,21,0};
        return star[row];
    }
    case '!': return row == 1 || row == 2 || row == 3 || row == 4 ? 4 :
                     row == 6 ? 4 : 0;
    case ',': return row == 6 ? 8 : 0;
    case '_': return row == 6 ? 31 : 0;
    case '.': return row == 6 ? 4 : 0;
    case ':': return row == 2 || row == 5 ? 4 : 0;
    case '/': return static_cast<uint8_t>(1u << (row / 2));
    case '?': {
        static const uint8_t q[7] = {14,17,1,2,4,0,4};
        return q[row];
    }
    case ' ': return 0;
    default: return 0;
    }
}

BGERenderer::BGERenderer(const BGEFramebuffer &framebuffer)
    : target(framebuffer) {}

bool BGERenderer::valid() const {
    return bge_framebuffer_valid(target);
}

const BGEFramebuffer &BGERenderer::framebuffer() const { return target; }

uint32_t BGERenderer::pixel(int x, int y) const {
    if (!valid() || x < 0 || y < 0 ||
        static_cast<uint32_t>(x) >= target.width ||
        static_cast<uint32_t>(y) >= target.height) return 0;
    return target.pixels[static_cast<size_t>(y) * target.stride_pixels +
                         static_cast<uint32_t>(x)];
}

uint32_t BGERenderer::blend_pixels(uint32_t background, uint32_t foreground,
                                   uint8_t alpha) {
    return bge_blend_pixel(background, foreground, alpha);
}

void BGERenderer::clear(uint32_t color) {
    if (!valid()) return;
    for (uint32_t y = 0; y < target.height; ++y) {
        std::fill(target.pixels + static_cast<size_t>(y) * target.stride_pixels,
                  target.pixels + static_cast<size_t>(y) * target.stride_pixels +
                      target.width,
                  color);
    }
}

void BGERenderer::draw_pixel(int x, int y, uint32_t color) {
    if (!valid() || x < 0 || y < 0 ||
        static_cast<uint32_t>(x) >= target.width ||
        static_cast<uint32_t>(y) >= target.height) return;
    target.pixels[static_cast<size_t>(y) * target.stride_pixels +
                  static_cast<uint32_t>(x)] = color;
}

void BGERenderer::draw_pixel_alpha(int x, int y, uint32_t color,
                                   uint8_t alpha) {
    if (alpha == 0 || !valid() || x < 0 || y < 0 ||
        static_cast<uint32_t>(x) >= target.width ||
        static_cast<uint32_t>(y) >= target.height) return;
    uint32_t &destination =
        target.pixels[static_cast<size_t>(y) * target.stride_pixels +
                      static_cast<uint32_t>(x)];
    destination = alpha == 255 ? color | 0xFF000000u
                               : blend_pixels(destination, color, alpha);
}

bool BGERenderer::clip_rect(int &x, int &y, int &width, int &height) const {
    if (!valid() || width <= 0 || height <= 0) return false;
    const int64_t right = static_cast<int64_t>(x) + width;
    const int64_t bottom = static_cast<int64_t>(y) + height;
    const int64_t clipped_left = std::max<int64_t>(0, x);
    const int64_t clipped_top = std::max<int64_t>(0, y);
    const int64_t clipped_right = std::min<int64_t>(target.width, right);
    const int64_t clipped_bottom = std::min<int64_t>(target.height, bottom);
    if (clipped_right <= clipped_left || clipped_bottom <= clipped_top) {
        return false;
    }
    x = static_cast<int>(clipped_left);
    y = static_cast<int>(clipped_top);
    width = static_cast<int>(clipped_right - clipped_left);
    height = static_cast<int>(clipped_bottom - clipped_top);
    return true;
}

void BGERenderer::fill_rect(int x, int y, int width, int height,
                            uint32_t color) {
    if (!clip_rect(x, y, width, height)) return;
    for (int row = y; row < y + height; ++row) {
        uint32_t *start = target.pixels +
                          static_cast<size_t>(row) * target.stride_pixels + x;
        std::fill(start, start + width, color);
    }
}

void BGERenderer::draw_gradient_rect(int x, int y, int width, int height,
                                     uint32_t top, uint32_t bottom) {
    if (!valid() || width <= 0 || height <= 0) return;
    const int original_y = y;
    const int original_height = height;
    if (!clip_rect(x, y, width, height)) return;
    for (int row = y; row < y + height; ++row) {
        const int64_t source_row =
            static_cast<int64_t>(row) - original_y;
        const uint32_t denominator = original_height > 1
                                         ? static_cast<uint32_t>(original_height - 1)
                                         : 1u;
        const uint32_t amount = static_cast<uint32_t>(
            static_cast<uint64_t>(source_row) * 255u / denominator);
        const uint32_t color =
            blend_pixels(top, bottom, static_cast<uint8_t>(amount));
        fill_rect(x, row, width, 1, color);
    }
}

void BGERenderer::draw_rounded_rect(int x, int y, int width, int height,
                                    int radius, uint32_t color, uint8_t alpha) {
    if (alpha == 0 || !valid() || width <= 0 || height <= 0) return;
    radius = std::max(0, std::min(radius, std::min(width, height) / 2));
    if (radius == 0) {
        int clipped_x = x;
        int clipped_y = y;
        int clipped_width = width;
        int clipped_height = height;
        if (clip_rect(clipped_x, clipped_y, clipped_width, clipped_height)) {
            for (int row = clipped_y; row < clipped_y + clipped_height; ++row) {
                for (int column = clipped_x; column < clipped_x + clipped_width;
                     ++column) {
                    draw_pixel_alpha(column, row, color, alpha);
                }
            }
        }
        return;
    }
    const int64_t bottom = static_cast<int64_t>(y) + height;
    const int first_row = static_cast<int>(std::max<int64_t>(0, y));
    const int last_row = static_cast<int>(std::min<int64_t>(
        target.height, bottom));
    for (int row = first_row; row < last_row; ++row) {
        const int64_t local_y = static_cast<int64_t>(row) - y;
        int inset = 0;
        if (local_y < radius) {
            const int64_t dy = radius - local_y;
            inset = radius - static_cast<int>(std::sqrt(
                static_cast<double>(radius) * radius -
                static_cast<double>(dy) * dy));
        } else if (local_y >= height - radius) {
            const int64_t dy = local_y - (height - radius - 1);
            inset = radius - static_cast<int>(std::sqrt(
                static_cast<double>(radius) * radius -
                static_cast<double>(dy) * dy));
        }
        const int64_t left = std::max<int64_t>(
            0, static_cast<int64_t>(x) + inset);
        const int64_t right = std::min<int64_t>(
            target.width, static_cast<int64_t>(x) + width - inset);
        for (int64_t column = left; column < right; ++column) {
            draw_pixel_alpha(static_cast<int>(column), row, color, alpha);
        }
    }
}

void BGERenderer::draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
    if (!valid()) return;
    double start_x = x0;
    double start_y = y0;
    double end_x = x1;
    double end_y = y1;
    const double dx_clip = end_x - start_x;
    const double dy_clip = end_y - start_y;
    double lower = 0.0;
    double upper = 1.0;
    const double p[4] = {-dx_clip, dx_clip, -dy_clip, dy_clip};
    const double q[4] = {
        start_x,
        static_cast<double>(target.width - 1u) - start_x,
        start_y,
        static_cast<double>(target.height - 1u) - start_y
    };
    for (unsigned index = 0; index < 4; ++index) {
        if (p[index] == 0.0) {
            if (q[index] < 0.0) return;
            continue;
        }
        const double ratio = q[index] / p[index];
        if (p[index] < 0.0) lower = std::max(lower, ratio);
        else upper = std::min(upper, ratio);
        if (lower > upper) return;
    }
    const double original_x = start_x;
    const double original_y = start_y;
    start_x = original_x + lower * dx_clip;
    start_y = original_y + lower * dy_clip;
    end_x = original_x + upper * dx_clip;
    end_y = original_y + upper * dy_clip;
    x0 = static_cast<int>(std::round(start_x));
    y0 = static_cast<int>(std::round(start_y));
    x1 = static_cast<int>(std::round(end_x));
    y1 = static_cast<int>(std::round(end_y));
    int dx = std::abs(x1 - x0);
    const int sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0);
    const int sy = y0 < y1 ? 1 : -1;
    int64_t error = static_cast<int64_t>(dx) + dy;
    for (;;) {
        draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        const int64_t twice = 2 * error;
        if (twice >= dy) { error += dy; x0 += sx; }
        if (twice <= dx) { error += dx; y0 += sy; }
    }
}

void BGERenderer::draw_text(int x, int y, const char *text, uint32_t color,
                            unsigned scale) {
    if (text == nullptr || scale == 0 || scale > 8) return;
    int64_t cursor_x = x;
    int64_t cursor_y = y;
    size_t character_count = 0;
    for (const char *character = text;
         *character != '\0' && character_count < 4096;
         ++character, ++character_count) {
        if (*character == '\n') {
            cursor_x = x;
            cursor_y += static_cast<int64_t>(9 * scale);
            continue;
        }
        for (unsigned row = 0; row < 7; ++row) {
            const uint8_t bits = glyph_row(*character, row);
            for (unsigned column = 0; column < 5; ++column) {
                if ((bits & (1u << (4u - column))) != 0) {
                    const int64_t pixel_x =
                        cursor_x + static_cast<int64_t>(column * scale);
                    const int64_t pixel_y =
                        cursor_y + static_cast<int64_t>(row * scale);
                    if (pixel_x < std::numeric_limits<int>::min() ||
                        pixel_x > std::numeric_limits<int>::max() ||
                        pixel_y < std::numeric_limits<int>::min() ||
                        pixel_y > std::numeric_limits<int>::max()) continue;
                    fill_rect(static_cast<int>(pixel_x),
                              static_cast<int>(pixel_y),
                              static_cast<int>(scale), static_cast<int>(scale),
                              color);
                }
            }
        }
        cursor_x += static_cast<int>(6 * scale);
    }
}

void BGERenderer::draw_icon(int x, int y, int size, uint32_t color,
                            const char *icon_name) {
    if (icon_name == nullptr || size < 8 || size > 4096 ||
        x > std::numeric_limits<int>::max() - size ||
        y > std::numeric_limits<int>::max() - size ||
        x < std::numeric_limits<int>::min() + size ||
        y < std::numeric_limits<int>::min() + size) return;
    const int unit = std::max(1, size / 16);
    const int left = x + size / 4;
    const int top = y + size / 4;
    if (std::strncmp(icon_name, "folder", 6) == 0) {
        draw_rounded_rect(left, top + unit * 3, size / 2, size / 2,
                          unit * 2, color);
        fill_rect(left + unit, top + unit, size / 4, unit * 3, color);
        return;
    }
    if (std::strcmp(icon_name, "system-search") == 0) {
        const int cx = x + size / 2 - unit;
        const int cy = y + size / 2 - unit;
        const int radius = std::max(2, size / 4);
        const int px[] = {cx, cx + radius, cx + radius * 2,
                          cx + radius * 2, cx + radius, cx};
        const int py[] = {cy - radius, cy - radius, cy, cy + radius,
                          cy + radius * 2, cy + radius * 2};
        for (unsigned index = 0; index < 5; ++index) {
            draw_line(px[index], py[index], px[index + 1], py[index + 1],
                      color);
        }
        draw_line(cx + radius + 1, cy + radius + 1,
                  x + size - unit, y + size - unit, color);
        return;
    }
    if (std::strcmp(icon_name, "network-wireless") == 0 ||
        std::strcmp(icon_name, "network-off") == 0) {
        for (int level = 0; level < 3; ++level) {
            const int inset = level * unit * 2;
            draw_line(x + size / 2, top + inset, left + size / 2 - inset,
                      y + size / 2 + inset, color);
            draw_line(x + size / 2, top + inset,
                      x + size / 2 + inset, y + size / 2 + inset, color);
        }
        fill_rect(x + size / 2 - unit, y + size - 2 * unit,
                  unit * 2, unit * 2, color);
        if (std::strcmp(icon_name, "network-off") == 0) {
            draw_line(left, top, x + size - unit, y + size - unit, color);
            draw_line(x + size - unit, top, left, y + size - unit, color);
        }
        return;
    }
    if (std::strcmp(icon_name, "preferences-system") == 0) {
        const int center = size / 2;
        draw_rounded_rect(x + center - unit * 2, y + center - unit * 2,
                          unit * 4, unit * 4, unit * 2, color);
        fill_rect(x + center - unit * 2, y + center - unit / 2,
                  unit * 4, unit, color);
        fill_rect(x + center - unit / 2, y + center - unit * 2,
                  unit, unit * 4, color);
        return;
    }
    if (std::strcmp(icon_name, "battery") == 0) {
        const int body_x = x + unit;
        const int body_y = y + size / 3;
        const int body_width = size - unit * 3;
        const int body_height = size / 2;
        fill_rect(body_x, body_y, body_width, unit, color);
        fill_rect(body_x, body_y + body_height - unit, body_width, unit, color);
        fill_rect(body_x, body_y, unit, body_height, color);
        fill_rect(body_x + body_width - unit, body_y, unit, body_height, color);
        fill_rect(body_x + body_width, body_y + body_height / 3,
                  unit * 2, body_height / 3, color);
        return;
    }
    if (std::strcmp(icon_name, "preferences-system-notifications") == 0) {
        draw_line(x + size / 2, top, left, y + size - unit * 3, color);
        draw_line(x + size / 2, top, x + size - unit, y + size - unit * 3,
                  color);
        fill_rect(left, y + size - unit * 3, size - size / 2, unit, color);
        fill_rect(x + size / 2 - unit, y + size - unit * 2,
                  unit * 2, unit, color);
        return;
    }
    if (std::strcmp(icon_name, "media-playback-start") == 0) {
        draw_line(left + unit, top, x + size - unit, y + size / 2, color);
        draw_line(x + size - unit, y + size / 2,
                  left + unit, y + size - unit, color);
        draw_line(left + unit, top, left + unit, y + size - unit, color);
        return;
    }
    if (std::strcmp(icon_name, "media-playback-pause") == 0) {
        fill_rect(left + unit, top, unit * 2, size / 2, color);
        fill_rect(left + unit * 4, top, unit * 2, size / 2, color);
        return;
    }
    if (std::strcmp(icon_name, "user-identity") == 0) {
        draw_rounded_rect(x + size / 3, top, size / 3, size / 3,
                          size / 6, color);
        draw_rounded_rect(left, y + size / 2, size - size / 2,
                          size / 3, size / 6, color);
        return;
    }
    if (std::strcmp(icon_name, "system-lock-screen") == 0) {
        draw_line(x + size / 3, top + size / 4,
                  x + size / 3, top, color);
        draw_line(x + size / 3, top, x + size * 2 / 3, top, color);
        draw_line(x + size * 2 / 3, top,
                  x + size * 2 / 3, top + size / 4, color);
        fill_rect(left, y + size / 2, size - size / 2, size / 3, color);
        return;
    }
    if (std::strcmp(icon_name, "system-software-install") == 0) {
        fill_rect(x + size / 2 - unit, top, unit * 2, size / 3, color);
        draw_line(x + size / 4, y + size / 2,
                  x + size / 2, y + size - unit, color);
        draw_line(x + size / 2, y + size - unit,
                  x + size * 3 / 4, y + size / 2, color);
        fill_rect(left, y + size - unit, size - size / 2, unit, color);
        return;
    }
    if (std::strcmp(icon_name, "text-x-generic") == 0 ||
        std::strcmp(icon_name, "text-plain") == 0 ||
        std::strcmp(icon_name, "image-x-generic") == 0 ||
        std::strcmp(icon_name, "audio-x-generic") == 0 ||
        std::strcmp(icon_name, "video-x-generic") == 0 ||
        std::strcmp(icon_name, "application-x-executable") == 0) {
        fill_rect(left, top, unit, size / 2, color);
        fill_rect(left, top, size / 2, unit, color);
        fill_rect(left + size / 2 - unit, top, unit, size / 2, color);
        fill_rect(left, top + size / 2 - unit, size / 2, unit, color);
        fill_rect(left + unit * 2, top + size / 3, size / 4, unit, color);
        fill_rect(left + unit * 2, top + size / 2, size / 4, unit, color);
        return;
    }
    if (std::strcmp(icon_name, "window-minimize") == 0) {
        fill_rect(x + size / 4, y + size * 2 / 3, size / 2, unit, color);
        return;
    }
    if (std::strcmp(icon_name, "window-maximize") == 0) {
        fill_rect(left, top, size / 2, unit, color);
        fill_rect(left, top, unit, size / 2, color);
        fill_rect(left + size / 2 - unit, top, unit, size / 2, color);
        fill_rect(left, top + size / 2 - unit, size / 2, unit, color);
        return;
    }
    if (std::strcmp(icon_name, "window-close") == 0) {
        draw_line(left, top, left + size / 2, top + size / 2, color);
        draw_line(left + size / 2, top, left, top + size / 2, color);
        return;
    }
    draw_rounded_rect(left, top, size / 2, size / 2, unit * 2, color);
}
