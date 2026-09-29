
#include "BAURA_DISPLAY_SERVER.h"
#include <vector>
#include <string>

class BauraTaskbar {
public:
    struct TaskItem {
        uint32_t app_id;
        std::string name;
        bool active;
    };

    // LEFT SECTION: Start Button + Dynamic App List
    void draw_left_section() {
        // Render Start Button (Mac-style Blur)
        render_blur_button(0, 0, "BAURA");

        // Render dynamic app list (Windows-style taskbar)
        int offset = 50;
        for (auto& app : open_apps) {
            render_app_tile(offset, 0, app.name);
            offset += 40;
        }
    }

    // RIGHT SECTION: System Tray (Battery, Net, Sound, Time)
    void draw_right_section() {
        int offset = 1920;
        render_icon(offset - 20, 0, "BATTERY");
        render_icon(offset - 40, 0, "NET");
        render_icon(offset - 60, 0, "SOUND");
        render_text(offset - 120, 0, "12:00 | Paris");
    }

private:
    std::vector<TaskItem> open_apps;
    void render_blur_button(int x, int y, std::string text) { /* MacOS Style Blur */ }
    void render_app_tile(int x, int y, std::string name) { /* Windows Style Tile */ }
    void render_icon(int x, int y, std::string icon) { /* Minimalist Icon */ }
    void render_text(int x, int y, std::string text) { /* San Francisco-like Font */ }
};
