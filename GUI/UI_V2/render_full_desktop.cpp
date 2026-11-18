#include "GUI/BGE/renderer.h"
#include "GUI/UI_V2/baura_shell.h"
#include "GUI/UI_V2/baura_apps.h"
#include "GUI/UI_V2/baura_boot_splash_v2.h"
#include "GUI/UI_V2/inter_bgf_data.h"
#include "GUI/UI_V2/mono_bgf_data.h"

#include <vector>
#include <fstream>
#include <iostream>

int main() {
    const int W = 1024;
    const int H = 768;
    std::vector<uint32_t> staging(W * H, 0xFF120F3A);

    BGEFramebuffer bge_fb{staging.data(), (uint32_t)W, (uint32_t)H, (uint32_t)W, staging.size()};
    BGERenderer renderer(bge_fb);
    BauraVec::Painter painter(renderer);

    BauraVec::Font font;
    if (!font.load(inter_bgf_data, inter_bgf_data_len)) {
        std::cerr << "Failed to load Inter font!" << std::endl;
        return 1;
    }

    BauraVec::Font mono;
    if (!mono.load(mono_bgf_data, mono_bgf_data_len)) {
        std::cerr << "Failed to load Mono font!" << std::endl;
        return 1;
    }

    BauraUI::Ui ui{painter, font, BauraBrand::DARK, false};
    BauraApps::Ctx ctx{ui, mono};

    // 1. Wallpaper
    BauraShell::wallpaper(ui, W, H, true);

    // 2. Window 1: Files
    BauraShell::window_frame(ui, {40, 50, 560, 440}, "Fichiers — Documents", "folder", true, false, 0);
    BauraApps::files(ctx, {40, 50 + 46, 560, 440 - 46});

    // 3. Window 2: Terminal
    BauraShell::window_frame(ui, {440, 160, 530, 380}, "Terminal", "terminal", true, true, 0);
    BauraApps::terminal(ctx, {440, 160 + 46, 530, 380 - 46});

    // 4. Split Taskbar & Top Tray
    BauraShell::TaskbarState tb_state;
    tb_state.n = 6;
    tb_state.pinned[0] = "files";
    tb_state.pinned[1] = "houb";
    tb_state.pinned[2] = "terminal";
    tb_state.pinned[3] = "media";
    tb_state.pinned[4] = "security";
    tb_state.pinned[5] = "settings";
    tb_state.active = 2; // terminal active
    tb_state.open_mask = 0b000101; // files + terminal open

    BauraShell::taskbar(ui, W, H, tb_state);
    BauraShell::tray(ui, W, "41°C", 2);

    static auto write_bmp = [](const char* path, const uint32_t* data, int width, int height) {
        uint32_t file_size = 54 + width * height * 4;
        int32_t neg_h = -height; // top-down
        uint8_t h[54] = {
            'B', 'M',
            (uint8_t)(file_size), (uint8_t)(file_size >> 8), (uint8_t)(file_size >> 16), (uint8_t)(file_size >> 24),
            0, 0, 0, 0,
            54, 0, 0, 0,
            40, 0, 0, 0,
            (uint8_t)(width), (uint8_t)(width >> 8), (uint8_t)(width >> 16), (uint8_t)(width >> 24),
            (uint8_t)(neg_h), (uint8_t)(neg_h >> 8), (uint8_t)(neg_h >> 16), (uint8_t)(neg_h >> 24),
            1, 0,
            32, 0,
            0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        };
        FILE* f = fopen(path, "wb");
        if (!f) return;
        fwrite(h, 1, 54, f);
        fwrite(data, 4, width * height, f);
        fclose(f);
    };

    write_bmp("/tmp/test_full_desktop.bmp", staging.data(), W, H);
    write_bmp("/mnt/c/Users/SDA/Downloads/bge_real_desktop.bmp", staging.data(), W, H);
    write_bmp("/mnt/c/Users/SDA/.gemini/antigravity/brain/ee2114af-8f40-4f20-a039-3567a4c7272d/bge_real_desktop.bmp", staging.data(), W, H);
    std::cout << "Full desktop rendered to BMP!" << std::endl;

    return 0;
}
