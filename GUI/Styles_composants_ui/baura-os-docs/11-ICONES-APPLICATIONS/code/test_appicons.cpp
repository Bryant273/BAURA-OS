// Planches de contrôle des icônes d'applications BAURA : catalogue complet, échelles 16→128, clair/sombre, résolution des identifiants.
#include <cstdio>
#include <string>
#include <vector>
#include "baura_appicons.h"
#include "baura_text.h"
#include "bmp_out.h"
using namespace BauraVec;
static int fails = 0;
#define CHECK(c) do { if (!(c)) { std::printf("ECHEC: %s (ligne %d)\n", #c, __LINE__); ++fails; } } while (0)
int main(int argc, char** argv) {
  (void)argc; (void)argv; auto blob = read_file("../../common/inter.bgf"); Font font; bool hasfont = font.load(blob.data(), blob.size());
  // résolution
  CHECK(appicons::find("files") && appicons::find("firefox") == appicons::find("browser"));
  CHECK(appicons::find("vlc") == appicons::find("video-player"));
  CHECK(appicons::find("org.gnome.Nautilus") == appicons::find("files"));
  CHECK(appicons::find("application-pdf") == appicons::find("file-pdf") && appicons::find("evince") == appicons::find("pdf-viewer"));
  CHECK(appicons::find("n-existe-pas") == nullptr && std::strcmp(appicons::resolve("n-existe-pas")->id, "app-generic") == 0);
  const size_t N = appicons::COUNT; const int cols = 12, cell = 104, rows = int((N + cols - 1) / cols);
  for (int dark = 0; dark < 2; ++dark) {
    int W = cols * cell + 20, H = rows * (cell + 6) + 150;
    std::vector<uint32_t> px(size_t(W) * H); BGEFramebuffer fb{px.data(), uint32_t(W), uint32_t(H), uint32_t(W), px.size()}; BGERenderer rd(fb); Painter p(rd);
    uint32_t bg = dark ? 0xFF14122E : 0xFFF3F4FB, fg = dark ? 0xFFE7E9FF : 0xFF1E1B4B; p.fill_rrect({0, 0, float(W), float(H)}, 0, Paint(bg));
    for (size_t i = 0; i < N; ++i) {
      int x = 10 + int(i % cols) * cell, y = 10 + int(i / cols) * (cell + 6);
      appicons::draw(p, appicons::ALL[i].id, {float(x + 12), float(y), 80, 80});
      if (hasfont) { std::string n = font.ellipsize(appicons::ALL[i].name, cell - 8, 12); font.draw(p, x + (cell - font.measure(n, 12)) / 2, float(y + 96), n, 12, fg); }
    }
    // échelles
    int y0 = rows * (cell + 6) + 20; const int sizes[] = {16, 20, 24, 32, 48, 64, 96};
    const char* demo[] = {"settings", "browser", "monitor", "folder", "file-pdf", "code-editor", "video-player", "game"};
    int x = 20; for (const char* id : demo) { int xx = x; for (int s : sizes) { appicons::draw(p, id, {float(xx), float(y0 + 100 - s), float(s), float(s)}); xx += s + 4; } x = xx + 16; if (x > W - 300) break; }
    write_ppm(dark ? "../render/appicons_sombre.ppm" : "../render/appicons_clair.ppm", px, W, H);
  }
  std::printf("%zu icônes, %d échec(s)\n", N, fails); return fails ? 1 : 0;
}
