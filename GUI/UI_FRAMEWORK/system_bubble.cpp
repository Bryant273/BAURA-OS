#include "system_bubble.h"

#include "baura_brand.h"
#include "baura_theme.h"

#include <algorithm>
#include <cmath>

BauraSystemBubble::BauraSystemBubble()
    : label(), progress(0.0f), visible(false) {}

void BauraSystemBubble::update_progress(const std::string &text, float value) {
    label = text;
    if (!std::isfinite(value)) value = 0.0f;
    progress = std::max(0.0f, std::min(1.0f, value));
    visible = true;
}

void BauraSystemBubble::set_visible(bool value) { visible = value; }

void BauraSystemBubble::render(BGERenderer &renderer, int x, int y,
                               int width) {
    if (!visible || !renderer.valid() || width < 120) return;
    renderer.draw_rounded_rect(x, y, width, 78, 16,
                               BauraTheme::GlassDark, 245);
    BauraBrand::draw_mark(renderer, x + 16, y + 14, 34);
    renderer.draw_text(x + 60, y + 22, label.c_str(),
                       BauraTheme::TextPrimary);
    renderer.draw_rounded_rect(x + 18, y + 42, width - 36, 10, 5,
                               BauraTheme::SurfaceRaised);
    const int progress_width =
        static_cast<int>((width - 36) * progress);
    if (progress_width > 0) {
        renderer.draw_rounded_rect(x + 18, y + 42, progress_width, 10, 5,
                                   BauraTheme::AccentBlue);
    }
}
