#ifndef BAURA_DESKTOP_H
#define BAURA_DESKTOP_H

#include "../BGE/renderer.h"
#include "../UI_FRAMEWORK/BAURA_UI_FRAMEWORK.h"
#include "../UI_FRAMEWORK/baura_hybrid_taskbar.h"

#include <vector>

class BauraDesktop {
public:
    BauraDesktop();
    BauraTaskbar &taskbar();
    bool add_window(BauraWindow *window);
    bool remove_window(BauraWindow *window);
    void render_frame(BGERenderer &renderer, float animation_phase);
    void process_click(int x, int y);
    void process_event(EventType type, int x, int y);

private:
    BauraTaskbar system_taskbar;
    std::vector<BauraWindow *> windows;
    uint32_t screen_width;
    uint32_t screen_height;
    BauraWindow *captured_window;
};

#endif
