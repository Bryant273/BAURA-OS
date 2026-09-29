
#ifndef BAURA_WINDOW_MANAGER_H
#define BAURA_WINDOW_MANAGER_H

#include "BAURA_COMPOSITOR.h"
#include <vector>
#include <string>

enum class WindowState { NORMAL, MINIMIZED, MAXIMIZED, FULLSCREEN };

class WindowManager {
public:
    WindowManager(WindowCompositor* compositor);
    uint32_t create_window(const std::string& title, int w, int h);
    void set_focus(uint32_t win_id);
    void resize_window(uint32_t win_id, int w, int h);
    void move_window(uint32_t win_id, int x, int y);
    void close_window(uint32_t win_id);

private:
    WindowCompositor* comp;
    struct WindowMeta {
        uint32_t id;
        std::string title;
        WindowState state;
        bool is_focused;
    };
    std::vector<WindowMeta> window_stack;
};
#endif
