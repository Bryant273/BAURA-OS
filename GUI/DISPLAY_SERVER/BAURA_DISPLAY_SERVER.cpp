#include "BAURA_DISPLAY_SERVER.h"

#include <algorithm>
#include <limits>
#include <new>
#include <utility>
#include <climits>

static const size_t max_surface_pixels = 16777216u;

DisplayServer::DisplayServer()
    : next_surface_id(1),
      screen_fb(nullptr),
      screen_width(0),
      screen_height(0),
      screen_stride(0),
      surface_pixels(0),
      present(nullptr),
      present_context(nullptr) {}

DisplayServer::~DisplayServer() = default;

int DisplayServer::set_framebuffer(uint32_t *pixels, uint32_t width,
                                   uint32_t height, uint32_t stride_pixels,
                                   size_t pixel_count,
                                   DisplayPresent present_callback,
                                   void *context) {
    if (pixels == nullptr || width == 0 || height == 0 ||
        width > static_cast<uint32_t>(INT_MAX) ||
        height > static_cast<uint32_t>(INT_MAX) || stride_pixels < width) {
        return DISPLAY_ERR_ARGUMENT;
    }
    if (static_cast<size_t>(height) >
        std::numeric_limits<size_t>::max() / stride_pixels) {
        return DISPLAY_ERR_ARGUMENT;
    }
    const size_t required_pixels =
        static_cast<size_t>(height) * stride_pixels;
    if (pixel_count < required_pixels) return DISPLAY_ERR_ARGUMENT;
    screen_fb = pixels;
    screen_width = width;
    screen_height = height;
    screen_stride = stride_pixels;
    present = present_callback;
    present_context = context;
    return DISPLAY_OK;
}

uint32_t DisplayServer::create_surface(int width, int height) {
    if (width <= 0 || height <= 0) return 0;
    const size_t surface_width = static_cast<size_t>(width);
    const size_t surface_height = static_cast<size_t>(height);
    if (surface_height > std::numeric_limits<size_t>::max() / surface_width) {
        return 0;
    }
    const size_t pixel_count = surface_width * surface_height;
    if (pixel_count > std::numeric_limits<size_t>::max() / sizeof(uint32_t)) {
        return 0;
    }
    if (pixel_count > max_surface_pixels - surface_pixels) return 0;
    std::unique_ptr<uint32_t[]> buffer(new (std::nothrow) uint32_t[pixel_count]());
    if (!buffer || next_surface_id == 0) return 0;

    const uint32_t id = next_surface_id++;
    Surface surface{id, static_cast<uint32_t>(width),
                    static_cast<uint32_t>(height), std::move(buffer)};
    try {
        surfaces.emplace(id, std::move(surface));
    } catch (const std::bad_alloc &) {
        return 0;
    }
    surface_pixels += pixel_count;
    return id;
}

int DisplayServer::update_surface(uint32_t id, const uint32_t *data,
                                  size_t pixel_count) {
    auto surface = surfaces.find(id);
    if (surface == surfaces.end()) return DISPLAY_ERR_NOT_FOUND;
    const size_t expected = static_cast<size_t>(surface->second.width) *
                            surface->second.height;
    if (data == nullptr || pixel_count != expected) {
        return DISPLAY_ERR_ARGUMENT;
    }
    for (size_t index = 0; index < expected; ++index) {
        surface->second.buffer[index] = data[index];
    }
    return DISPLAY_OK;
}

int DisplayServer::destroy_surface(uint32_t id) {
    auto surface = surfaces.find(id);
    if (surface == surfaces.end()) return DISPLAY_ERR_NOT_FOUND;
    surface_pixels -= static_cast<size_t>(surface->second.width) *
                      surface->second.height;
    surfaces.erase(surface);
    return DISPLAY_OK;
}

int DisplayServer::resize_surface(uint32_t id, int width, int height) {
    auto found = surfaces.find(id);
    if (found == surfaces.end()) return DISPLAY_ERR_NOT_FOUND;
    if (width <= 0 || height <= 0) return DISPLAY_ERR_ARGUMENT;
    const size_t new_width = static_cast<size_t>(width);
    const size_t new_height = static_cast<size_t>(height);
    if (new_height > std::numeric_limits<size_t>::max() / new_width) {
        return DISPLAY_ERR_ARGUMENT;
    }
    const size_t new_count = new_width * new_height;
    const size_t old_count = static_cast<size_t>(found->second.width) *
                             found->second.height;
    if (new_count > max_surface_pixels - (surface_pixels - old_count)) {
        return DISPLAY_ERR_CAPACITY;
    }
    std::unique_ptr<uint32_t[]> resized(
        new (std::nothrow) uint32_t[new_count]());
    if (!resized) return DISPLAY_ERR_CAPACITY;
    const uint32_t copy_width = std::min<uint32_t>(
        found->second.width, static_cast<uint32_t>(width));
    const uint32_t copy_height = std::min<uint32_t>(
        found->second.height, static_cast<uint32_t>(height));
    for (uint32_t row = 0; row < copy_height; ++row) {
        std::copy_n(found->second.buffer.get() +
                        static_cast<size_t>(row) * found->second.width,
                    copy_width,
                    resized.get() + static_cast<size_t>(row) * new_width);
    }
    found->second.width = static_cast<uint32_t>(width);
    found->second.height = static_cast<uint32_t>(height);
    found->second.buffer = std::move(resized);
    surface_pixels = surface_pixels - old_count + new_count;
    return DISPLAY_OK;
}

int DisplayServer::clear(uint32_t color) {
    if (screen_fb == nullptr) return DISPLAY_ERR_UNAVAILABLE;
    for (uint32_t y = 0; y < screen_height; ++y) {
        for (uint32_t x = 0; x < screen_width; ++x) {
            screen_fb[static_cast<size_t>(y) * screen_stride + x] = color;
        }
    }
    return DISPLAY_OK;
}

int DisplayServer::draw_surface(uint32_t id, int x, int y) {
    auto found = surfaces.find(id);
    if (found == surfaces.end()) return DISPLAY_ERR_NOT_FOUND;
    if (screen_fb == nullptr) return DISPLAY_ERR_UNAVAILABLE;

    const Surface &surface = found->second;
    const int64_t source_left = std::max<int64_t>(0, -static_cast<int64_t>(x));
    const int64_t source_top = std::max<int64_t>(0, -static_cast<int64_t>(y));
    const int64_t source_right = std::min<int64_t>(
        surface.width, static_cast<int64_t>(screen_width) - x);
    const int64_t source_bottom = std::min<int64_t>(
        surface.height, static_cast<int64_t>(screen_height) - y);
    if (source_right <= source_left || source_bottom <= source_top) {
        return DISPLAY_OK;
    }
    for (int64_t source_y = source_top; source_y < source_bottom; ++source_y) {
        const size_t target_y = static_cast<size_t>(
            static_cast<int64_t>(y) + source_y);
        for (int64_t source_x = source_left; source_x < source_right;
             ++source_x) {
            const size_t target_x = static_cast<size_t>(
                static_cast<int64_t>(x) + source_x);
            const uint32_t source = surface.buffer[
                static_cast<size_t>(source_y) * surface.width +
                static_cast<size_t>(source_x)];
            const size_t target_index =
                target_y * screen_stride + target_x;
            if ((source >> 24) == 0xFFu) {
                screen_fb[target_index] = source;
                continue;
            }
            const uint32_t alpha = source >> 24;
            const uint32_t background = screen_fb[target_index];
            const uint32_t inverse = 255u - alpha;
            uint32_t result = 0xFF000000u;
            for (uint32_t shift = 0; shift <= 16; shift += 8) {
                const uint32_t foreground_channel = (source >> shift) & 0xFFu;
                const uint32_t background_channel =
                    (background >> shift) & 0xFFu;
                const uint32_t channel =
                    (foreground_channel * alpha +
                     background_channel * inverse + 127u) /
                    255u;
                result |= channel << shift;
            }
            screen_fb[target_index] = result;
        }
    }
    return DISPLAY_OK;
}

int DisplayServer::flip_buffer() {
    if (screen_fb == nullptr) return DISPLAY_ERR_UNAVAILABLE;
    if (present != nullptr &&
        present(present_context, screen_fb, screen_width, screen_height,
                screen_stride) != 0) {
        return DISPLAY_ERR_UNAVAILABLE;
    }
    return DISPLAY_OK;
}

bool DisplayServer::has_surface(uint32_t id) const {
    return surfaces.find(id) != surfaces.end();
}

bool DisplayServer::surface_size(uint32_t id, uint32_t &surface_width,
                                 uint32_t &surface_height) const {
    const auto surface = surfaces.find(id);
    if (surface == surfaces.end()) return false;
    surface_width = surface->second.width;
    surface_height = surface->second.height;
    return true;
}

uint32_t DisplayServer::width() const { return screen_width; }
uint32_t DisplayServer::height() const { return screen_height; }
