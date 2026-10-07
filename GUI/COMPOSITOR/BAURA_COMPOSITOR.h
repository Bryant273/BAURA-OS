
#ifndef BAURA_COMPOSITOR_H
#define BAURA_COMPOSITOR_H

#include "BAURA_DISPLAY_SERVER.h"
#include <vector>

struct Window {
    uint32_t id;
    uint32_t surface_id;
    int x, y, z;
    bool visible;
};

class WindowCompositor {
public:
    WindowCompositor(DisplayServer* ds);

    uint32_t add_window(uint32_t surface_id, int x, int y);
    int set_z_order(uint32_t win_id, int z);
    int move_window(uint32_t win_id, int x, int y);
    int set_visible(uint32_t win_id, bool visible);
    int set_surface(uint32_t win_id, uint32_t surface_id);
    int remove_window(uint32_t win_id);
    int compose();

private:
    DisplayServer* server;
    uint32_t next_window_id;
    std::vector<Window> windows;
};

#endif
