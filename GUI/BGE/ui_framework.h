#ifndef BAURA_BGE_UI_FRAMEWORK_H
#define BAURA_BGE_UI_FRAMEWORK_H

#include "renderer.h"
#include "window_manager.h"
#include "../UI_FRAMEWORK/baura_hybrid_taskbar.h"

enum class BGEPointerEvent { PRESS, MOVE, RELEASE };

class BGEUIFramework {
public:
    explicit BGEUIFramework(BGERenderer &renderer);
    uint32_t create_window(const std::string &title, BGERect bounds);
    bool close_window(uint32_t id);
    bool handle_click(int x, int y);
    bool handle_pointer(BGEPointerEvent event, int x, int y);
    void render_desktop(float animation_phase);
    BGEWindowManager &windows();
    BauraTaskbar &taskbar();

private:
    BGERenderer &renderer;
    BGEWindowManager window_manager;
    BauraTaskbar system_taskbar;
};

#endif
