#ifndef BAURA_BRAND_H
#define BAURA_BRAND_H

#include "../BGE/renderer.h"
#include "baura_theme.h"

#include <algorithm>
#include <cstdint>
#include <limits>

namespace BauraBrand {

inline void draw_mark(BGERenderer &renderer, int x, int y, int size,
                      uint8_t alpha = 255) {
    if (!renderer.valid() || size < 12 || size > 4096 ||
        x > std::numeric_limits<int>::max() - size ||
        y > std::numeric_limits<int>::max() - size ||
        x < std::numeric_limits<int>::min() + size ||
        y < std::numeric_limits<int>::min() + size) return;

    renderer.draw_rounded_rect(x, y, size, size, size / 3,
                               BauraTheme::AccentBlue, alpha);
    const int cell = std::max(1, size / 8);
    const int left = x + (size - cell * 5) / 2;
    const int top = y + (size - cell * 7) / 2;
    static constexpr uint8_t glyph[7] = {
        0x1Eu, 0x11u, 0x11u, 0x1Eu, 0x11u, 0x11u, 0x1Eu
    };
    for (int row = 0; row < 7; ++row) {
        const uint32_t color = row < 3 ? BauraTheme::White
                                      : BauraTheme::TextPrimary;
        for (int column = 0; column < 5; ++column) {
            if ((glyph[row] & (1u << (4 - column))) == 0) continue;
            for (int dy = 0; dy < cell; ++dy) {
                for (int dx = 0; dx < cell; ++dx) {
                    renderer.draw_pixel_alpha(left + column * cell + dx,
                                              top + row * cell + dy,
                                              color, alpha);
                }
            }
        }
    }
    const int spark = std::max(1, size / 16);
    for (int dy = 0; dy < spark; ++dy) {
        for (int dx = 0; dx < spark; ++dx) {
            renderer.draw_pixel_alpha(x + size - spark * 3 + dx,
                                      y + spark * 2 + dy,
                                      BauraTheme::AccentCyan, alpha);
        }
    }
}

inline void draw_lockup(BGERenderer &renderer, int x, int y, int mark_size,
                        uint32_t text_color = BauraTheme::TextPrimary,
                        unsigned text_scale = 2) {
    if (mark_size < 12 || mark_size > 4096 || text_scale == 0 ||
        text_scale > 8 ||
        mark_size < static_cast<int>(7u * text_scale)) return;
    const int gap = std::max(8, mark_size / 5);
    const int text_width = 5 * 6 * static_cast<int>(text_scale);
    if (x > std::numeric_limits<int>::max() - mark_size - gap - text_width ||
        x < std::numeric_limits<int>::min() + mark_size ||
        y > std::numeric_limits<int>::max() - mark_size ||
        y < std::numeric_limits<int>::min() + mark_size) return;
    draw_mark(renderer, x, y, mark_size);
    const int text_x = x + mark_size + gap;
    renderer.draw_text(
        text_x, y + (mark_size - 7 * static_cast<int>(text_scale)) / 2,
        "BAURA", text_color, text_scale);
}

inline void draw_boot_splash(BGERenderer &renderer,
                             const char *status = "STARTING SYSTEM SERVICES") {
    if (!renderer.valid() ||
        renderer.framebuffer().width >
            static_cast<uint32_t>(std::numeric_limits<int>::max()) ||
        renderer.framebuffer().height >
            static_cast<uint32_t>(std::numeric_limits<int>::max())) return;
    const BGEFramebuffer &fb = renderer.framebuffer();
    const int width = static_cast<int>(fb.width);
    const int height = static_cast<int>(fb.height);
    renderer.draw_gradient_rect(0, 0, width, height, 0xFF101827u,
                                0xFF1A2944u);
    const int panel_width = std::min(640, width - 32);
    const int panel_height = std::min(300, height - 32);
    if (panel_width < 240 || panel_height < 268) return;
    const int panel_x = (width - panel_width) / 2;
    const int panel_y = (height - panel_height) / 2;
    renderer.draw_rounded_rect(panel_x, panel_y, panel_width, panel_height,
                               28, BauraTheme::GlassDark, 225);
    const int lockup_width = 112 + 24 + 8 * 12;
    const int lockup_x = panel_x + (panel_width - lockup_width) / 2;
    const int lockup_y = panel_y + 62;
    draw_mark(renderer, lockup_x, lockup_y, 112);
    renderer.draw_text(lockup_x + 136, lockup_y + 43, "BAURA OS",
                       BauraTheme::TextPrimary, 2);
    char bounded_status[64];
    const char *source = status == nullptr
                             ? "STARTING SYSTEM SERVICES"
                             : status;
    size_t status_length = 0;
    while (status_length < sizeof(bounded_status) - 1 &&
           source[status_length] != '\0') {
        bounded_status[status_length] = source[status_length];
        ++status_length;
    }
    bounded_status[status_length] = '\0';
    renderer.draw_text(panel_x + 38, panel_y + 210, bounded_status,
                       BauraTheme::TextSecondary);
    renderer.draw_rounded_rect(panel_x + 38, panel_y + 246,
                               panel_width - 76, 6, 3,
                               BauraTheme::SurfaceRaised);
    renderer.draw_rounded_rect(panel_x + 38, panel_y + 246,
                               (panel_width - 76) * 2 / 5, 6, 3,
                               BauraTheme::AccentCyan);
}

}

#endif
