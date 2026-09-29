
#include "BAURA_WINDOW_MANAGER.h"

WindowManager::WindowManager(WindowCompositor* compositor) : comp(compositor) {}

uint32_t WindowManager::create_window(const std::string& title, int w, int h) {
    uint32_t surface_id = gui_create_surface(global_ds, w, h);
    uint32_t win_id = comp->add_window(surface_id, 100, 100);
    window_stack.push_back({win_id, title, WindowState::NORMAL, true});
    return win_id;
}

void WindowManager::set_focus(uint32_t win_id) {
    for(auto& win : window_stack) {
        win.is_focused = (win.id == win_id);
    }
    comp->set_z_order(win_id, 999);
}
