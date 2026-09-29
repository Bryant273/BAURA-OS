 #include "BAURA_WINDOW_MANAGER.h"
#include "BAURA_UI_FRAMEWORK.h"

void WindowManager::process_input(int mx, int my, EventType type) {
    for (auto it = window_stack.rbegin(); it != window_stack.rend(); ++it) {
        if (it->is_focused) {
            it->handle_event(type, mx, my);
        }
    }
}