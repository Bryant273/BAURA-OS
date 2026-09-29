
#ifndef BAURA_COMPOSITOR_H
#define BAURA_COMPOSITOR_H

#include "BAURA_DISPLAY_SERVER.h"
#include <vector>

struct Window {
    uint32_t surface_id;
    int x, y, z;
    bool visible;
};

class WindowCompositor {
public:
    WindowCompositor(DisplayServer* ds);

    uint32_t add_window(uint32_t surface_id, int x, int y);
    void set_z_order(uint32_t win_id, int z);
    void compose();

private:
    DisplayServer* server;
    std::vector<Window> windows;
};

#endif
