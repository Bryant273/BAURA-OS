
#include "BAURA_COMPOSITOR.h"
#include <algorithm>
#include <iostream>

WindowCompositor::WindowCompositor(DisplayServer* ds) : server(ds) {}

uint32_t WindowCompositor::add_window(uint32_t surface_id, int x, int y) {
    uint32_t id = windows.size();
    windows.push_back({surface_id, x, y, 0, true});
    return id;
}

void WindowCompositor::set_z_order(uint32_t win_id, int z) {
    if (win_id < windows.size()) {
        windows[win_id].z = z;
    }
}

void WindowCompositor::compose() {
    // 1. Sort windows by Z-order (Back to Front)
    std::sort(windows.begin(), windows.end(), [](const Window& a, const Window& b) {
        return a.z < b.z;
    });

    // 2. Blend surfaces into the final framebuffer
    for (auto& win : windows) {
        if (win.visible) {
            std::cout << "[Compositor] Rendering window " << win.surface_id
                      << " at (" << win.x << "," << win.y << ") with Z=" << win.z << std::endl;
        }
    }
    server->flip_buffer();
}
