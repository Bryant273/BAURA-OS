#ifndef BAURA_BGE_WINDOW_MANAGER_H
#define BAURA_BGE_WINDOW_MANAGER_H

#include "renderer.h"

#include <cstdint>
#include <string>
#include <vector>

struct BGEWindow {
    uint32_t id;
    std::string title;
    BGERect bounds;
    BGERect restore_bounds;
    bool focused;
    bool minimized;
    bool maximized;
};

class BGEWindowManager {
public:
    explicit BGEWindowManager(BGERenderer &renderer);
    uint32_t create_window(const std::string &title, BGERect bounds);
    bool close_window(uint32_t id);
    bool set_focus(uint32_t id);
    bool minimize(uint32_t id);
    bool toggle_maximize(uint32_t id);
    bool restore(uint32_t id);
    bool move_window(uint32_t id, int x, int y);
    bool drag_to(int x, int y);
    void end_drag();
    uint32_t window_at(int x, int y) const;
    bool handle_click(int x, int y);
    void composite();

private:
    BGEWindow *find(uint32_t id);
    const BGEWindow *find(uint32_t id) const;
    BGERenderer &renderer;
    uint32_t next_id;
    uint32_t dragging_id;
    int drag_offset_x;
    int drag_offset_y;
    std::vector<BGEWindow> windows;
};

#endif
