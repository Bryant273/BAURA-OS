#include "ui_extensions.h"

#include <algorithm>
#include <cmath>
#include <limits>

void render_performance_graph(BGERenderer &renderer, int x, int y, int width,
                              int height, const float *data, size_t length,
                              uint32_t color) {
    if (!renderer.valid() || data == nullptr || length < 2 ||
        width < 2 || height < 2) return;
    const BGEFramebuffer &fb = renderer.framebuffer();
    const int64_t right = static_cast<int64_t>(x) + width;
    const int64_t bottom = static_cast<int64_t>(y) + height;
    const int visible_left = static_cast<int>(std::max<int64_t>(0, x));
    const int visible_right = static_cast<int>(std::min<int64_t>(fb.width, right));
    const int visible_top = static_cast<int>(std::max<int64_t>(0, y));
    const int visible_bottom =
        static_cast<int>(std::min<int64_t>(fb.height, bottom));
    if (visible_right <= visible_left || visible_bottom <= visible_top) return;
    renderer.draw_rounded_rect(x, y, width, height, 8, 0xA0182432u, 255);
    int previous_x = visible_left;
    int previous_y = visible_top;
    bool have_previous = false;
    const uint64_t span = static_cast<uint64_t>(length - 1);
    const uint64_t denominator = static_cast<uint64_t>(width - 1);
    for (int point_x = visible_left; point_x < visible_right; ++point_x) {
        const uint64_t column = static_cast<uint64_t>(
            static_cast<int64_t>(point_x) - x);
        const uint64_t sample =
            (span / denominator) * column +
            ((span % denominator) * column) / denominator;
        float value = data[sample];
        if (!std::isfinite(value)) value = 0;
        value = std::max(0.0f, std::min(1.0f, value));
        const int64_t graph_y =
            bottom - 1 - static_cast<int64_t>(value * (height - 1));
        const int point_y = static_cast<int>(std::max<int64_t>(
            std::numeric_limits<int>::min(),
            std::min<int64_t>(std::numeric_limits<int>::max(), graph_y)));
        if (have_previous) {
            renderer.draw_line(previous_x, previous_y, point_x, point_y, color);
        }
        previous_x = point_x;
        previous_y = point_y;
        have_previous = true;
    }
}
