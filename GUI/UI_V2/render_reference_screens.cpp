#include "GUI/BGE/renderer.h"
#include "GUI/UI_V2/baura_shell.h"
#include "GUI/UI_V2/baura_boot_splash_v2.h"
#include "GUI/UI_V2/inter_bgf_data.h"

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

    // 1. Render Reference Boot Splash
    BauraUI::Ui ui{painter, font, BauraBrand::DARK, false};
    BauraBoot::draw_splash(ui, W, H, 9.0f, 0.65f, "Démarrage des services système...", true);

    // Save PPM for boot splash
    std::ofstream out_boot("/tmp/test_boot_v2.ppm", std::ios::binary);
    out_boot << "P6\n" << W << " " << H << "\n255\n";
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            uint32_t c = staging[y * W + x];
            uint8_t r = (c >> 16) & 0xFF;
            uint8_t g = (c >> 8) & 0xFF;
            uint8_t b = c & 0xFF;
            out_boot.put(r).put(g).put(b);
        }
    }
    out_boot.close();
    std::cout << "Boot splash v2 rendered to /tmp/test_boot_v2.ppm!" << std::endl;

    // 2. Render Reference Desktop
    BauraShell::wallpaper(ui, W, H, true);
    BauraShell::TaskbarState tb_state;
    tb_state.n = 6;
    tb_state.pinned[0] = "files";
    tb_state.pinned[1] = "houb";
    tb_state.pinned[2] = "terminal";
    tb_state.pinned[3] = "media";
    tb_state.pinned[4] = "security";
    tb_state.pinned[5] = "settings";
    tb_state.active = 0;
    tb_state.open_mask = 0b000101; // files & terminal open

    BauraShell::taskbar(ui, W, H, tb_state);
    BauraShell::tray(ui, W, "41°C", 2);

    std::ofstream out_desk("/tmp/test_desk_v2.ppm", std::ios::binary);
    out_desk << "P6\n" << W << " " << H << "\n255\n";
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            uint32_t c = staging[y * W + x];
            uint8_t r = (c >> 16) & 0xFF;
            uint8_t g = (c >> 8) & 0xFF;
            uint8_t b = c & 0xFF;
            out_desk.put(r).put(g).put(b);
        }
    }
    out_desk.close();
    std::cout << "Desktop v2 rendered to /tmp/test_desk_v2.ppm!" << std::endl;

    return 0;
}
