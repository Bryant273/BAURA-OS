#include "gpu_accel_3d.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <new>

BGE3DRenderer::BGE3DRenderer(BGERenderer &target, uint32_t target_width,
                             uint32_t target_height)
    : renderer(target), width(target_width), height(target_height) {
    if (target_width == 0 || target_height == 0 ||
        static_cast<size_t>(target_width) >
            std::numeric_limits<size_t>::max() / target_height ||
        static_cast<size_t>(target_width) * target_height > 4194304u) {
        width = 0;
        height = 0;
        return;
    }
    try {
        depth_buffer.resize(static_cast<size_t>(target_width) * target_height,
                            1.0f);
    } catch (const std::bad_alloc &) {
        width = 0;
        height = 0;
    }
}

bool BGE3DRenderer::valid() const {
    const BGEFramebuffer &fb = renderer.framebuffer();
    return renderer.valid() && width != 0 && height != 0 &&
           width <= fb.width && height <= fb.height && !depth_buffer.empty();
}

void BGE3DRenderer::clear_depth(float depth) {
    if (!std::isfinite(depth) || depth < 0.0f || depth > 1.0f) return;
    std::fill(depth_buffer.begin(), depth_buffer.end(), depth);
}

static float edge(const BGEVertex &a, const BGEVertex &b, float x, float y) {
    return (x - a.x) * (b.y - a.y) - (y - a.y) * (b.x - a.x);
}

bool BGE3DRenderer::draw_triangle(const BGETriangle &triangle) {
    if (!valid()) return false;
    const BGEVertex *vertices[] = {
        &triangle.first, &triangle.second, &triangle.third
    };
    for (const BGEVertex *vertex : vertices) {
        if (!std::isfinite(vertex->x) || !std::isfinite(vertex->y) ||
            !std::isfinite(vertex->depth) || vertex->depth < 0.0f ||
            vertex->depth > 1.0f) return false;
    }
    const float minimum_x = std::min(
        triangle.first.x, std::min(triangle.second.x, triangle.third.x));
    const float maximum_x = std::max(
        triangle.first.x, std::max(triangle.second.x, triangle.third.x));
    const float minimum_y = std::min(
        triangle.first.y, std::min(triangle.second.y, triangle.third.y));
    const float maximum_y = std::max(
        triangle.first.y, std::max(triangle.second.y, triangle.third.y));
    if (maximum_x < 0.0f || maximum_y < 0.0f ||
        minimum_x >= static_cast<float>(width) ||
        minimum_y >= static_cast<float>(height)) return true;
    const float area = edge(triangle.first, triangle.second,
                            triangle.third.x, triangle.third.y);
    if (area == 0.0f || !std::isfinite(area)) return true;
    const int min_x = static_cast<int>(
        std::floor(std::max(0.0, static_cast<double>(minimum_x))));
    const int min_y = static_cast<int>(
        std::floor(std::max(0.0, static_cast<double>(minimum_y))));
    const int max_x = static_cast<int>(std::ceil(std::min(
        static_cast<double>(width - 1), static_cast<double>(maximum_x))));
    const int max_y = static_cast<int>(std::ceil(std::min(
        static_cast<double>(height - 1), static_cast<double>(maximum_y))));
    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {
            const float px = static_cast<float>(x) + 0.5f;
            const float py = static_cast<float>(y) + 0.5f;
            const float w0 = edge(triangle.second, triangle.third, px, py) / area;
            const float w1 = edge(triangle.third, triangle.first, px, py) / area;
            const float w2 = edge(triangle.first, triangle.second, px, py) / area;
            if (w0 < 0 || w1 < 0 || w2 < 0) continue;
            const float depth = w0 * triangle.first.depth +
                                w1 * triangle.second.depth +
                                w2 * triangle.third.depth;
            if (!std::isfinite(depth)) return false;
            const size_t index = static_cast<size_t>(y) * width +
                                 static_cast<uint32_t>(x);
            if (depth < depth_buffer[index]) {
                depth_buffer[index] = depth;
                const uint32_t colors[] = {
                    triangle.first.color, triangle.second.color,
                    triangle.third.color
                };
                const float weights[] = {w0, w1, w2};
                uint32_t color = 0;
                for (unsigned shift = 0; shift <= 24; shift += 8) {
                    const float channel =
                        weights[0] * ((colors[0] >> shift) & 0xFFu) +
                        weights[1] * ((colors[1] >> shift) & 0xFFu) +
                        weights[2] * ((colors[2] >> shift) & 0xFFu);
                    const uint32_t value = static_cast<uint32_t>(
                        std::max(0.0f, std::min(255.0f, std::round(channel))));
                    color |= value << shift;
                }
                renderer.draw_pixel(x, y, color);
            }
        }
    }
    return true;
}
