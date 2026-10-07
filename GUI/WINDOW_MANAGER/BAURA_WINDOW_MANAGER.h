#ifndef BAURA_WINDOW_MANAGER_H
#define BAURA_WINDOW_MANAGER_H

#include "../COMPOSITOR/BAURA_COMPOSITOR.h"

#include <string>
#include <vector>

enum class WindowState { INVALID, NORMAL, MINIMIZED, MAXIMIZED, FULLSCREEN };
enum class BauraWindowInputEvent { CLICK, HOVER, DRAG, RESIZE };
typedef void (*BauraWindowInputHandler)(uint32_t window_id,
                                        BauraWindowInputEvent event,
                                        int x, int y, void *context);

class WindowManager {
public:
    WindowManager(DisplayServer *display, WindowCompositor *compositor);
    uint32_t create_window(const std::string &title, int width, int height);
    int set_focus(uint32_t window_id);
    int resize_window(uint32_t window_id, int width, int height);
    int move_window(uint32_t window_id, int x, int y);
    int set_state(uint32_t window_id, WindowState state);
    int restore_window(uint32_t window_id);
    WindowState state(uint32_t window_id) const;
    int close_window(uint32_t window_id);
    int set_input_handler(uint32_t window_id, BauraWindowInputHandler handler,
                          void *context);
    void process_input(int x, int y, BauraWindowInputEvent event);

private:
    DisplayServer *display;
    WindowCompositor *compositor;
    struct WindowMeta {
        uint32_t id;
        uint32_t surface_id;
        std::string title;
        WindowState state;
        bool is_focused;
        int x;
        int y;
        int width;
        int height;
        int restore_x;
        int restore_y;
        int restore_width;
        int restore_height;
        BauraWindowInputHandler input_handler;
        void *input_context;
    };
    std::vector<WindowMeta> window_stack;
};

#endif
