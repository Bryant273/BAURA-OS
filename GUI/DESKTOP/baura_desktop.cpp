#include "baura_desktop.h"

#include "../UI_FRAMEWORK/baura_theme.h"

#include <algorithm>
#include <cmath>
#include <new>

BauraDesktop::BauraDesktop()
    : screen_width(0), screen_height(0), captured_window(nullptr) {}

BauraTaskbar &BauraDesktop::taskbar() { return system_taskbar; }

bool BauraDesktop::add_window(BauraWindow *window) {
    if (window == nullptr ||
        std::find(windows.begin(), windows.end(), window) != windows.end()) {
        return false;
    }
    try {
        windows.push_back(window);
    } catch (const std::bad_alloc &) {
        return false;
    }
    return true;
}

bool BauraDesktop::remove_window(BauraWindow *window) {
    if (captured_window == window) captured_window = nullptr;
    auto found = std::find(windows.begin(), windows.end(), window);
    if (found == windows.end()) return false;
    windows.erase(found);
    return true;
}

void BauraDesktop::render_frame(BGERenderer &renderer, float animation_phase) {
    if (!renderer.valid()) return;
    const BGEFramebuffer &fb = renderer.framebuffer();
    screen_width = fb.width;
    screen_height = fb.height;
    const int work_height = static_cast<int>(fb.height) - 58;
    if (work_height > 0) {
        for (BauraWindow *window : windows) {
            if (window != nullptr) {
                window->set_work_area(0, 0, static_cast<int>(fb.width),
                                      work_height);
            }
        }
    }
    const float phase = std::isfinite(animation_phase)
                            ? animation_phase
                            : 0.0f;
    const uint32_t blend = static_cast<uint32_t>(
        (std::sin(phase) + 1.0f) * 0.5f * 255.0f);
    const uint32_t upper = BGERenderer::blend_pixels(
        0xFF101827u, 0xFF233B62u, static_cast<uint8_t>(blend / 3u));
    const uint32_t lower = BGERenderer::blend_pixels(
        0xFF182D3Bu, 0xFF34254Bu, static_cast<uint8_t>(blend / 3u));
    renderer.draw_gradient_rect(0, 0, static_cast<int>(fb.width),
                                static_cast<int>(fb.height), upper, lower);
    const int glow_width = static_cast<int>(fb.width / 3u);
    const int glow_height = static_cast<int>(fb.height / 2u);
    renderer.draw_rounded_rect(static_cast<int>(fb.width) / 2 - glow_width / 2,
                               static_cast<int>(fb.height) / 4, glow_width,
                               glow_height, glow_width / 2,
                               BauraTheme::AccentBlue, 18);
    for (BauraWindow *window : windows) {
        if (window != nullptr) window->draw(renderer);
    }
    system_taskbar.render(renderer);
}

void BauraDesktop::process_click(int x, int y) {
    process_event(EventType::CLICK, x, y);
}

void BauraDesktop::process_event(EventType type, int x, int y) {
    if (type == EventType::DRAG && captured_window != nullptr) {
        captured_window->handle_event(type, x, y);
        return;
    }
    if (type == EventType::RELEASE) {
        if (captured_window != nullptr) {
            captured_window->handle_event(type, x, y);
            captured_window = nullptr;
        }
        return;
    }
    if (type != EventType::CLICK) {
        for (auto window = windows.rbegin(); window != windows.rend(); ++window) {
            if (*window != nullptr && !(*window)->closed() &&
                !(*window)->minimized() && (*window)->contains(x, y)) {
                (*window)->handle_event(type, x, y);
                return;
            }
        }
        return;
    }
    if (screen_width != 0 && screen_height != 0 &&
        system_taskbar.handle_click(x, y, screen_width, screen_height)) {
        captured_window = nullptr;
        return;
    }
    for (auto window = windows.rbegin(); window != windows.rend(); ++window) {
        if (*window != nullptr && !(*window)->closed() &&
            !(*window)->minimized() && (*window)->contains(x, y)) {
            for (BauraWindow *candidate : windows) {
                if (candidate != nullptr) candidate->set_focused(candidate == *window);
            }
            (*window)->handle_event(EventType::CLICK, x, y);
            captured_window = (*window)->is_dragging() ? *window : nullptr;
            return;
        }
    }
    for (BauraWindow *window : windows) {
        if (window != nullptr) window->set_focused(false);
    }
    captured_window = nullptr;
}
