// Compare le verre actuel du dépôt (BGEEffects::draw_glass_panel) au verre BAURA (baura_glass.h) sur le même fond.
#include "baura_glass.h"
#include "effects_engine.h"
#include "bmp_out.h"
#include <chrono>
using namespace BauraVec;
static void scene(Painter& p, int W, int H, float ox) {   // fond riche : dégradé + symbole géant + points colorés
  p.fill_rrect({ox, 0, float(W), float(H)}, 0, Paint::linear(150, 0xFFF1E4FF, 0xFF8B7BF5));
  p.symbol({ox + 80, -20, float(W) * .30f, float(H) * 1.15f}, 0xFF4F46E5, 0xFF8B5CF6, 0xFFC4B5FD, 0xFFAB8E18);
  p.fill_rrect({ox + 40, 260, 120, 120}, 60, 0xFFE0BE3A); p.fill_rrect({ox + 330, 40, 90, 90}, 45, 0xFFFB7185); p.fill_rrect({ox + 250, 300, 130, 40}, 20, 0xFF17133D);
}
int main() {
  const int W = 960, H = 440, half = 480; std::vector<uint32_t> px(size_t(W) * H); BGEFramebuffer fb{px.data(), W, H, W, px.size()}; BGERenderer r(fb); Painter p(r);
  scene(p, half, H, 0); scene(p, half, H, half);
  // GAUCHE : verre actuel du dépôt
  BGEEffects fx(r); fx.draw_glass_panel(60, 100, 360, 240, 22, 0xFFFFFFFF, 90);
  // DROITE : verre BAURA (ombre + flou gaussien σ=18 + saturation 1,9 + teinte + bordure + reflet)
  GlassStyle g; auto t0 = std::chrono::steady_clock::now();
  shadow(p, {half + 60, 100, 360, 240}, 22, 12, 34, 0x38342890); shadow(p, {half + 60, 100, 360, 240}, 22, 2, 6, 0x1A342890); glass(p, {half + 60, 100, 360, 240}, 22, g);
  double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
  p.fill_rrect({0, 0, 2, float(H)}, 0, 0xFFFFFFFF); std::printf("verre BAURA 360×240 : %.1f ms (CPU, 1 cœur)\n", ms);
  write_ppm("glass.ppm", px, W, H);
}
