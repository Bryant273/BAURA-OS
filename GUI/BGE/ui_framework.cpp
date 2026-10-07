#include "ui_framework.h"

#include "../UI_FRAMEWORK/baura_theme.h"

#include <cmath>

BGEUIFramework::BGEUIFramework(BGERenderer &target)
    : renderer(target), window_manager(target) {}

uint32_t BGEUIFramework::create_window(const std::string &title,
                                       BGERect bounds) {
    return window_manager.create_window(title, bounds);
}

bool BGEUIFramework::close_window(uint32_t id) {
    return window_manager.close_window(id);
}

bool BGEUIFramework::handle_click(int x, int y) {
    const BGEFramebuffer &fb = renderer.framebuffer();
    if (renderer.valid() &&
        system_taskbar.handle_click(x, y, fb.width, fb.height)) return true;
    return window_manager.handle_click(x, y);
}

bool BGEUIFramework::handle_pointer(BGEPointerEvent event, int x, int y) {
    if (event == BGEPointerEvent::PRESS) return handle_click(x, y);
    if (event == BGEPointerEvent::MOVE) return window_manager.drag_to(x, y);
    window_manager.end_drag();
    return true;
}

void BGEUIFramework::render_desktop(float animation_phase) {
    if (!renderer.valid()) return;
    const BGEFramebuffer &fb = renderer.framebuffer();
    const float phase = std::isfinite(animation_phase) ? animation_phase : 0.0f;
    const uint8_t blend = static_cast<uint8_t>(
        (std::sin(phase) + 1.0f) * 42.0f);
    const uint32_t top = BGERenderer::blend_pixels(
        BauraTheme::Background, 0xFF233B62u, blend);
    const uint32_t bottom = BGERenderer::blend_pixels(
        0xFF182D3Bu, 0xFF34254Bu, blend);
    renderer.draw_gradient_rect(0, 0, static_cast<int>(fb.width),
                                static_cast<int>(fb.height), top, bottom);
    window_manager.composite();
    system_taskbar.render(renderer);
}

BGEWindowManager &BGEUIFramework::windows() { return window_manager; }
BauraTaskbar &BGEUIFramework::taskbar() { return system_taskbar; }
