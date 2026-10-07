#include "BAURA_COMPOSITOR.h"

#include <algorithm>
#include <new>

WindowCompositor::WindowCompositor(DisplayServer *ds)
    : server(ds), next_window_id(1) {}

uint32_t WindowCompositor::add_window(uint32_t surface_id, int x, int y) {
    if (server == nullptr || surface_id == 0 ||
        !server->has_surface(surface_id) || next_window_id == 0) {
        return 0;
    }
    const uint32_t id = next_window_id;
    try {
        windows.push_back({id, surface_id, x, y, 0, true});
    } catch (const std::bad_alloc &) {
        return 0;
    }
    ++next_window_id;
    return id;
}

int WindowCompositor::set_z_order(uint32_t win_id, int z) {
    for (Window &window : windows) {
        if (window.id == win_id) {
            window.z = z;
            return DISPLAY_OK;
        }
    }
    return DISPLAY_ERR_NOT_FOUND;
}

int WindowCompositor::move_window(uint32_t win_id, int x, int y) {
    for (Window &window : windows) {
        if (window.id == win_id) {
            window.x = x;
            window.y = y;
            return DISPLAY_OK;
        }
    }
    return DISPLAY_ERR_NOT_FOUND;
}

int WindowCompositor::set_visible(uint32_t win_id, bool visible) {
    for (Window &window : windows) {
        if (window.id == win_id) {
            window.visible = visible;
            return DISPLAY_OK;
        }
    }
    return DISPLAY_ERR_NOT_FOUND;
}

int WindowCompositor::set_surface(uint32_t win_id, uint32_t surface_id) {
    if (server == nullptr || !server->has_surface(surface_id)) {
        return DISPLAY_ERR_NOT_FOUND;
    }
    for (Window &window : windows) {
        if (window.id == win_id) {
            window.surface_id = surface_id;
            return DISPLAY_OK;
        }
    }
    return DISPLAY_ERR_NOT_FOUND;
}

int WindowCompositor::remove_window(uint32_t win_id) {
    auto window = std::find_if(windows.begin(), windows.end(),
                               [win_id](const Window &candidate) {
                                   return candidate.id == win_id;
                               });
    if (window == windows.end()) return DISPLAY_ERR_NOT_FOUND;
    windows.erase(window);
    return DISPLAY_OK;
}

int WindowCompositor::compose() {
    if (server == nullptr) return DISPLAY_ERR_ARGUMENT;
    for (const Window &window : windows) {
        if (window.visible && !server->has_surface(window.surface_id)) {
            return DISPLAY_ERR_NOT_FOUND;
        }
    }
    int status = server->clear(0xFF000000u);
    if (status != DISPLAY_OK) return status;

    std::sort(windows.begin(), windows.end(),
                     [](const Window &left, const Window &right) {
                         return left.z != right.z ? left.z < right.z
                                                  : left.id < right.id;
                     });
    for (const Window &window : windows) {
        if (!window.visible) continue;
        status = server->draw_surface(window.surface_id, window.x, window.y);
        if (status != DISPLAY_OK) return status;
    }
    return server->flip_buffer();
}
