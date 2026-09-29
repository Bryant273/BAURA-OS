
#include "window_manager.cpp"

class BGEUIFramework {
    BGEWindowManager wm;
    BGERenderer renderer;

public:
    void render_desktop() {
        // 1. Draw Wallpaper (Simple gradient for now)
        renderer.draw_gradient_rect(0, 0, baura_fb.width, baura_fb.height, 0x1a2a6c, 0xb21f1f, 255);

        // 2. Draw Windows
        wm.composite();

        // 3. Draw Split Taskbar (Bottom)
        uint32_t taskbar_h = 45;
        uint32_t taskbar_y = baura_fb.height - taskbar_h;

        // Taskbar Background (Glassmorphism)
        renderer.draw_rounded_rect(0, taskbar_y, baura_fb.width, taskbar_h, 0, 0x111111, 150);

        // Left Side: Start Button & Dynamic Apps
        renderer.draw_rounded_rect(10, taskbar_y + 5, 40, 35, 8, 0x0078D7, 255); // Start Button

        // Right Side: System Tray (Battery, Network, Clock)
        renderer.draw_rounded_rect(baura_fb.width - 150, taskbar_y + 5, 140, 35, 8, 0x333333, 200);
    }
};
