
#include <vector>
#include <string>
#include "renderer.cpp"

struct Window {
    uint32_t id;
    std::string title;
    uint32_t x, y, w, h;
    bool is_focused;
    bool is_minimized;
    uint32_t color;
};

class BGEWindowManager {
    std::vector<Window*> windows;
    BGERenderer renderer;
    uint32_t next_id = 1;

public:
    void create_window(std::string title, uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
        Window* win = new Window{next_id++, title, x, y, w, h, true, false, 0x222222};
        windows.push_back(win);
    }

    void composite() {
        // Draw windows from back to front (Z-order)
        for (auto win : windows) {
            if (win->is_minimized) continue;

            // 1. Draw the Glass Background
            renderer.draw_rounded_rect(win->x, win->y, win->w, win->h, 15, win->color, 180);

            // 2. Draw the Title Bar (Windows style but macOS look)
            renderer.draw_gradient_rect(win->x, win->y, win->w, 30, 0x444444, 0x222222, 255);

            // 3. Draw the Control Buttons (-, o, x)
            renderer.draw_rounded_rect(win->x + win->w - 80, win->y + 8, 15, 15, 5, 0xCCCCCC, 255); // Min
            renderer.draw_rounded_rect(win->x + win->w - 60, win->y + 8, 15, 15, 5, 0xCCCCCC, 255); // Max
            renderer.draw_rounded_rect(win->x + win->w - 40, win->y + 8, 15, 15, 5, 0xFF5555, 255); // Close
        }
    }

    void handle_click(uint32_t mx, uint32_t my) {
        // Check if a button was clicked or a window was dragged
        // Update Z-order by moving the focused window to the end of the vector
    }
};
