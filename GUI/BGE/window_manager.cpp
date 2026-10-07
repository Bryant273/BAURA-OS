#include "window_manager.h"

#include "../UI_FRAMEWORK/baura_theme.h"

#include <algorithm>
#include <climits>
#include <new>

static bool valid_bounds(const BGERect &bounds) {
    return bounds.width > 0 && bounds.height > 0 &&
           static_cast<int64_t>(bounds.x) + bounds.width <= INT_MAX &&
           static_cast<int64_t>(bounds.y) + bounds.height <= INT_MAX;
}

BGEWindowManager::BGEWindowManager(BGERenderer &target)
    : renderer(target), next_id(1), dragging_id(0), drag_offset_x(0),
      drag_offset_y(0) {}

uint32_t BGEWindowManager::create_window(const std::string &title,
                                         BGERect bounds) {
    if (title.empty() || bounds.width < 120 || bounds.height < 72 ||
        !valid_bounds(bounds) ||
        next_id == 0) return 0;
    const uint32_t id = next_id;
    try {
        windows.push_back({id, title, bounds, bounds, false, false, false});
    } catch (const std::bad_alloc &) {
        return 0;
    }
    ++next_id;
    set_focus(id);
    return id;
}

bool BGEWindowManager::close_window(uint32_t id) {
    const auto found = std::find_if(
        windows.begin(), windows.end(),
        [id](const BGEWindow &window) { return window.id == id; });
    if (found == windows.end()) return false;
    if (dragging_id == id) end_drag();
    windows.erase(found);
    for (auto window = windows.rbegin(); window != windows.rend(); ++window) {
        if (!window->minimized) {
            set_focus(window->id);
            break;
        }
    }
    return true;
}

BGEWindow *BGEWindowManager::find(uint32_t id) {
    for (BGEWindow &window : windows) {
        if (window.id == id) return &window;
    }
    return nullptr;
}

const BGEWindow *BGEWindowManager::find(uint32_t id) const {
    for (const BGEWindow &window : windows) {
        if (window.id == id) return &window;
    }
    return nullptr;
}

bool BGEWindowManager::set_focus(uint32_t id) {
    auto focused = std::find_if(
        windows.begin(), windows.end(),
        [id](const BGEWindow &window) { return window.id == id; });
    if (focused == windows.end() || focused->minimized) return false;
    for (BGEWindow &window : windows) window.focused = false;
    focused->focused = true;
    if (focused != windows.end() - 1) {
        std::rotate(focused, focused + 1, windows.end());
        windows.back().focused = true;
    }
    return true;
}

bool BGEWindowManager::minimize(uint32_t id) {
    BGEWindow *window = find(id);
    if (window == nullptr) return false;
    if (dragging_id == id) end_drag();
    window->minimized = true;
    window->focused = false;
    if (!windows.empty()) {
        for (auto candidate = windows.rbegin(); candidate != windows.rend();
             ++candidate) {
            if (!candidate->minimized) {
                set_focus(candidate->id);
                break;
            }
        }
    }
    return true;
}

bool BGEWindowManager::toggle_maximize(uint32_t id) {
    BGEWindow *window = find(id);
    if (window == nullptr) return false;
    if (window->minimized && !restore(id)) return false;
    window = find(id);
    if (window == nullptr) return false;
    if (window->maximized) return restore(id);
    if (dragging_id == id) end_drag();
    const BGEFramebuffer &fb = renderer.framebuffer();
    if (!renderer.valid() || fb.width == 0 || fb.height <= 58) return false;
    window->restore_bounds = window->bounds;
    window->bounds = {0, 0, static_cast<int>(fb.width),
                      static_cast<int>(fb.height) - 58};
    window->maximized = true;
    return true;
}

bool BGEWindowManager::restore(uint32_t id) {
    BGEWindow *window = find(id);
    if (window == nullptr) return false;
    if (window->maximized) {
        window->bounds = window->restore_bounds;
        window->maximized = false;
    }
    window->minimized = false;
    return set_focus(id);
}

bool BGEWindowManager::move_window(uint32_t id, int x, int y) {
    BGEWindow *window = find(id);
    if (window == nullptr || window->maximized ||
        !valid_bounds({x, y, window->bounds.width, window->bounds.height})) {
        return false;
    }
    window->bounds.x = x;
    window->bounds.y = y;
    if (!window->minimized) set_focus(id);
    return true;
}

bool BGEWindowManager::drag_to(int x, int y) {
    if (dragging_id == 0) return false;
    const int64_t next_x = static_cast<int64_t>(x) - drag_offset_x;
    const int64_t next_y = static_cast<int64_t>(y) - drag_offset_y;
    if (next_x < INT_MIN || next_x > INT_MAX ||
        next_y < INT_MIN || next_y > INT_MAX) return false;
    return move_window(dragging_id, static_cast<int>(next_x),
                       static_cast<int>(next_y));
}

void BGEWindowManager::end_drag() {
    dragging_id = 0;
    drag_offset_x = 0;
    drag_offset_y = 0;
}

uint32_t BGEWindowManager::window_at(int x, int y) const {
    for (auto window = windows.rbegin(); window != windows.rend(); ++window) {
        if (!window->minimized && x >= window->bounds.x &&
            y >= window->bounds.y &&
            static_cast<int64_t>(x) <
                static_cast<int64_t>(window->bounds.x) + window->bounds.width &&
            static_cast<int64_t>(y) <
                static_cast<int64_t>(window->bounds.y) + window->bounds.height) {
            return window->id;
        }
    }
    return 0;
}

bool BGEWindowManager::handle_click(int mouse_x, int mouse_y) {
    end_drag();
    const uint32_t id = window_at(mouse_x, mouse_y);
    if (id == 0 || !set_focus(id)) return false;
    const BGEWindow *window = find(id);
    if (window == nullptr || mouse_y < window->bounds.y ||
        static_cast<int64_t>(mouse_y) >=
            static_cast<int64_t>(window->bounds.y) + 40) return true;
    const int controls_x = window->bounds.x + window->bounds.width - 84;
    if (mouse_x >= controls_x && mouse_x < controls_x + 26) {
        return minimize(id);
    }
    if (mouse_x >= controls_x + 26 && mouse_x < controls_x + 52) {
        return toggle_maximize(id);
    }
    if (mouse_x >= controls_x + 52 && mouse_x < controls_x + 84) {
        return close_window(id);
    }
    if (mouse_y >= window->bounds.y &&
        static_cast<int64_t>(mouse_y) <
            static_cast<int64_t>(window->bounds.y) + 40) {
        dragging_id = id;
        drag_offset_x = mouse_x - window->bounds.x;
        drag_offset_y = mouse_y - window->bounds.y;
    }
    return true;
}

void BGEWindowManager::composite() {
    for (const BGEWindow &window : windows) {
        if (window.minimized) continue;
        const BGERect &rect = window.bounds;
        renderer.draw_rounded_rect(rect.x + 2, rect.y + 4, rect.width,
                                   rect.height, 14, BauraTheme::Shadow, 110);
        renderer.draw_rounded_rect(rect.x, rect.y, rect.width, rect.height,
                                   14, BauraTheme::GlassDark, 240);
        renderer.draw_rounded_rect(
            rect.x, rect.y, rect.width, 40, 14,
            window.focused ? BauraTheme::SurfaceRaised : BauraTheme::Surface);
        renderer.fill_rect(rect.x, rect.y + 28, rect.width, 12,
                           window.focused ? BauraTheme::SurfaceRaised
                                          : BauraTheme::Surface);
        renderer.draw_text(rect.x + 14, rect.y + 17, window.title.c_str(),
                           BauraTheme::TextPrimary);
        const int controls_x = rect.x + rect.width - 84;
        const uint32_t colors[] = {
            BauraTheme::Warning, BauraTheme::Success, BauraTheme::Danger
        };
        const char *icons[] = {
            "window-minimize", "window-maximize", "window-close"
        };
        for (int index = 0; index < 3; ++index) {
            const int control_x = controls_x + index * 26;
            renderer.draw_rounded_rect(control_x, rect.y + 8, 20, 20, 10,
                                       colors[index]);
            renderer.draw_icon(control_x + 3, rect.y + 11, 14,
                               BauraTheme::TextPrimary, icons[index]);
        }
    }
}
