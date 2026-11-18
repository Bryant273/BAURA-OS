// Planches de contact des 550 icônes (5 pages) — clair, avec le nom sous chaque icône.
#include "baura_text.h"
#include "baura_theme_brand.h"
#include "bmp_out.h"
using namespace BauraVec;
int main() {
  auto blob = read_file("../../common/inter.bgf"); Font f; f.load(blob.data(), blob.size()); const auto& T = BauraBrand::LIGHT;
  const int per = 110, cols = 10, cw = 110, chh = 66, W = cols * cw + 20, H = 11 * chh + 70;
  for (int page = 0; page < 5; ++page) {
    std::vector<uint32_t> px(size_t(W) * H); BGEFramebuffer fb{px.data(), W, H, W, px.size()}; BGERenderer r(fb); Painter p(r);
    p.fill_rrect({0, 0, float(W), float(H)}, 0, Paint::linear(150, 0xFFF7F1FF, 0xFFDCD3FF));
    char t[96]; std::snprintf(t, sizeof t, "BAURA OS — 550 icônes · page %d/5", page + 1); f.draw(p, 20, 36, t, 20, T.fg, Weight::SemiBold);
    for (int i = 0; i < per; ++i) { size_t k = size_t(page) * per + i; if (k >= icons::COUNT) break;
      float x = 10 + (i % cols) * cw, y = 56 + (i / cols) * chh; p.fill_rrect({x + 4, y, cw - 8, chh - 6}, 14, 0x99FFFFFF);
      p.icon(icons::ALL[k].name, {x + cw / 2.f - 14, y + 8, 28, 28}, T.fg);
      std::string n = f.ellipsize(icons::ALL[k].name, cw - 16, 12); float w = f.measure(n, 12); f.draw(p, x + (cw - w) / 2, y + 52, n, 12, T.mut); }
    std::snprintf(t, sizeof t, "icones_page%d.ppm", page + 1); write_ppm(t, px, W, H);
  }
}
