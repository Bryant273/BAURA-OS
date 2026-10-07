#include "BAURA_WINDOW_MANAGER.h"

#include <algorithm>
#include <new>

WindowManager::WindowManager(DisplayServer *display_server,
                             WindowCompositor *window_compositor)
    : display(display_server), compositor(window_compositor) {}

uint32_t WindowManager::create_window(const std::string &title, int width,
                                      int height) {
    if (display == nullptr || compositor == nullptr || title.empty()) return 0;
    const uint32_t surface_id = display->create_surface(width, height);
    if (surface_id == 0) return 0;
    const uint32_t window_id =
        compositor->add_window(surface_id, 100, 100);
    if (window_id == 0) {
        display->destroy_surface(surface_id);
        return 0;
    }
    try {
        window_stack.push_back({window_id, surface_id, title, WindowState::NORMAL,
                                false, 100, 100, width, height, 100, 100,
                                width, height, nullptr, nullptr});
    } catch (const std::bad_alloc &) {
        compositor->remove_window(window_id);
        display->destroy_surface(surface_id);
        return 0;
    }
    if (set_focus(window_id) != DISPLAY_OK) {
        compositor->remove_window(window_id);
        display->destroy_surface(surface_id);
        window_stack.erase(std::remove_if(
            window_stack.begin(), window_stack.end(),
            [window_id](const WindowMeta &window) {
                return window.id == window_id;
            }), window_stack.end());
        return 0;
    }
    return window_id;
}

int WindowManager::set_focus(uint32_t window_id) {
    if (compositor == nullptr) return DISPLAY_ERR_ARGUMENT;
    auto focused = std::find_if(
        window_stack.begin(), window_stack.end(),
        [window_id](const WindowMeta &window) { return window.id == window_id; });
    if (focused == window_stack.end()) return DISPLAY_ERR_NOT_FOUND;
    if (focused->state == WindowState::MINIMIZED) {
        return DISPLAY_ERR_ARGUMENT;
    }
    for (WindowMeta &window : window_stack) window.is_focused = false;
    focused->is_focused = true;
    if (focused != window_stack.end() - 1) {
        std::rotate(focused, focused + 1, window_stack.end());
        window_stack.back().is_focused = true;
    }
    for (size_t index = 0; index < window_stack.size(); ++index) {
        const int status = compositor->set_z_order(
            window_stack[index].id, static_cast<int>(index));
        if (status != DISPLAY_OK) return status;
    }
    return DISPLAY_OK;
}

int WindowManager::resize_window(uint32_t window_id, int width, int height) {
    if (display == nullptr || compositor == nullptr) {
        return DISPLAY_ERR_ARGUMENT;
    }
    auto window = std::find_if(
        window_stack.begin(), window_stack.end(),
        [window_id](const WindowMeta &candidate) {
            return candidate.id == window_id;
        });
    if (window == window_stack.end()) return DISPLAY_ERR_NOT_FOUND;
    const int status = display->resize_surface(window->surface_id, width, height);
    if (status != DISPLAY_OK) return status;
    window->width = width;
    window->height = height;
    if (window->state == WindowState::NORMAL) {
        window->restore_width = width;
        window->restore_height = height;
    }
    return DISPLAY_OK;
}

int WindowManager::move_window(uint32_t window_id, int x, int y) {
    if (compositor == nullptr) return DISPLAY_ERR_ARGUMENT;
    auto window = std::find_if(
        window_stack.begin(), window_stack.end(),
        [window_id](const WindowMeta &candidate) {
            return candidate.id == window_id;
        });
    if (window == window_stack.end()) return DISPLAY_ERR_NOT_FOUND;
    if (window->state == WindowState::MAXIMIZED ||
        window->state == WindowState::FULLSCREEN) {
        return DISPLAY_ERR_ARGUMENT;
    }
    const int result = compositor->move_window(window_id, x, y);
    if (result == DISPLAY_OK) {
        window->x = x;
        window->y = y;
        if (window->state == WindowState::NORMAL) {
            window->restore_x = x;
            window->restore_y = y;
        }
    }
    return result;
}

int WindowManager::set_state(uint32_t window_id, WindowState next_state) {
    if (display == nullptr || compositor == nullptr) {
        return DISPLAY_ERR_ARGUMENT;
    }
    if (next_state != WindowState::NORMAL &&
        next_state != WindowState::MINIMIZED &&
        next_state != WindowState::MAXIMIZED &&
        next_state != WindowState::FULLSCREEN) return DISPLAY_ERR_ARGUMENT;
    auto window = std::find_if(
        window_stack.begin(), window_stack.end(),
        [window_id](const WindowMeta &candidate) {
            return candidate.id == window_id;
        });
    if (window == window_stack.end()) return DISPLAY_ERR_NOT_FOUND;
    if (window->state == next_state) return DISPLAY_OK;

    if (next_state == WindowState::MINIMIZED) {
        const int status = compositor->set_visible(window_id, false);
        if (status == DISPLAY_OK) {
            window->state = next_state;
            window->is_focused = false;
            for (auto candidate = window_stack.rbegin();
                 candidate != window_stack.rend(); ++candidate) {
                if (candidate->state != WindowState::MINIMIZED) {
                    set_focus(candidate->id);
                    break;
                }
            }
        }
        return status;
    }
    if (window->state == WindowState::MINIMIZED) {
        const int status = compositor->set_visible(window_id, true);
        if (status != DISPLAY_OK) return status;
    }
    if (next_state == WindowState::NORMAL) {
        const int move_status = compositor->move_window(
            window_id, window->restore_x, window->restore_y);
        if (move_status != DISPLAY_OK) {
            if (window->state == WindowState::MINIMIZED) {
                const int hide_status =
                    compositor->set_visible(window_id, false);
                if (hide_status != DISPLAY_OK) return hide_status;
            }
            return move_status;
        }
        const int resize_status = resize_window(
            window_id, window->restore_width, window->restore_height);
        if (resize_status != DISPLAY_OK) {
            const int rollback_status =
                compositor->move_window(window_id, window->x, window->y);
            if (rollback_status != DISPLAY_OK) return rollback_status;
            if (window->state == WindowState::MINIMIZED) {
                const int hide_status =
                    compositor->set_visible(window_id, false);
                if (hide_status != DISPLAY_OK) return hide_status;
            }
            return resize_status;
        }
        window->x = window->restore_x;
        window->y = window->restore_y;
        window->state = WindowState::NORMAL;
        return set_focus(window_id);
    }
    const WindowState previous_state = window->state;
    if (previous_state == WindowState::NORMAL) {
        window->restore_x = window->x;
        window->restore_y = window->y;
        window->restore_width = window->width;
        window->restore_height = window->height;
    }
    const uint32_t screen_width = display->width();
    const uint32_t screen_height = display->height();
    if (screen_width == 0 || screen_height == 0) {
        if (previous_state == WindowState::MINIMIZED) {
            const int hide_status = compositor->set_visible(window_id, false);
            if (hide_status != DISPLAY_OK) return hide_status;
        }
        return DISPLAY_ERR_UNAVAILABLE;
    }
    window->state = next_state;
    const int status = compositor->move_window(window_id, 0, 0);
    if (status != DISPLAY_OK) {
        window->state = previous_state;
        if (previous_state == WindowState::MINIMIZED) {
            const int hide_status = compositor->set_visible(window_id, false);
            if (hide_status != DISPLAY_OK) return hide_status;
        }
        return status;
    }
    const int resize_status = resize_window(
        window_id, static_cast<int>(screen_width),
        static_cast<int>(screen_height));
    if (resize_status != DISPLAY_OK) {
        const int rollback_status =
            compositor->move_window(window_id, window->x, window->y);
        window->state = previous_state;
        if (rollback_status != DISPLAY_OK) return rollback_status;
        if (previous_state == WindowState::MINIMIZED) {
            const int hide_status = compositor->set_visible(window_id, false);
            if (hide_status != DISPLAY_OK) return hide_status;
        }
        return resize_status;
    }
    window->x = 0;
    window->y = 0;
    window->state = next_state;
    return set_focus(window_id);
}

int WindowManager::restore_window(uint32_t window_id) {
    return set_state(window_id, WindowState::NORMAL);
}

WindowState WindowManager::state(uint32_t window_id) const {
    const auto window = std::find_if(
        window_stack.begin(), window_stack.end(),
        [window_id](const WindowMeta &candidate) {
            return candidate.id == window_id;
        });
    return window == window_stack.end() ? WindowState::INVALID : window->state;
}

int WindowManager::close_window(uint32_t window_id) {
    if (display == nullptr || compositor == nullptr) {
        return DISPLAY_ERR_ARGUMENT;
    }
    auto window = std::find_if(
        window_stack.begin(), window_stack.end(),
        [window_id](const WindowMeta &candidate) {
            return candidate.id == window_id;
        });
    if (window == window_stack.end()) return DISPLAY_ERR_NOT_FOUND;
    const int status = compositor->remove_window(window_id);
    if (status != DISPLAY_OK) return status;
    const int destroy_status = display->destroy_surface(window->surface_id);
    window_stack.erase(window);
    for (auto candidate = window_stack.rbegin();
         candidate != window_stack.rend(); ++candidate) {
        if (candidate->state != WindowState::MINIMIZED) {
            set_focus(candidate->id);
            break;
        }
    }
    return destroy_status;
}

int WindowManager::set_input_handler(uint32_t window_id,
                                     BauraWindowInputHandler handler,
                                     void *context) {
    auto window = std::find_if(
        window_stack.begin(), window_stack.end(),
        [window_id](const WindowMeta &candidate) {
            return candidate.id == window_id;
        });
    if (window == window_stack.end()) return DISPLAY_ERR_NOT_FOUND;
    window->input_handler = handler;
    window->input_context = context;
    return DISPLAY_OK;
}

void WindowManager::process_input(int x, int y,
                                  BauraWindowInputEvent event) {
    bool hit_window = event != BauraWindowInputEvent::CLICK;
    if (event == BauraWindowInputEvent::CLICK) {
        for (auto window = window_stack.rbegin(); window != window_stack.rend();
             ++window) {
            if (window->state == WindowState::MINIMIZED || x < window->x ||
                y < window->y ||
                static_cast<int64_t>(x) >=
                    static_cast<int64_t>(window->x) + window->width ||
                static_cast<int64_t>(y) >=
                    static_cast<int64_t>(window->y) + window->height) continue;
            set_focus(window->id);
            hit_window = true;
            break;
        }
    }
    if (!hit_window) return;
    for (auto window = window_stack.rbegin(); window != window_stack.rend();
         ++window) {
        if (window->is_focused && window->state != WindowState::MINIMIZED &&
            window->input_handler != nullptr) {
            window->input_handler(window->id, event, x, y,
                                  window->input_context);
            return;
        }
    }
}
