// Séquences d'animation réelles : BGEWindowManager (dépôt) + WindowMotion (BAURA). Écrit 2 planches + courbes, et vérifie des invariants.
#include "baura_wm_motion.h"
#include "bmp_out.h"
#include <cassert>
using namespace BauraUI;
static const int TW = 560, TH = 340;
struct Scene {
  std::vector<uint32_t> px; BGEFramebuffer fb; BGERenderer r; Painter p; Ui u; BGEWindowManager wm; WindowMotion mo;
  Scene(const Font& f) : px(size_t(TW) * TH), fb{px.data(), TW, TH, TW, px.size()}, r(fb), p(r), u{p, f, BauraBrand::LIGHT}, wm(r) {
    mo.dock_rect = [](uint32_t id) { return BauraVec::Rect{60.f + 48 * ((id - 1) % 3), TH - 54.f, 40, 40}; };
    mo.style = [](uint32_t id, WindowMotion::Visual& v) { v.icon = id == 1 ? "folder" : "settings"; v.c1 = id == 1 ? 0xFF6D66F2 : 0xFF9AA0C8; v.c2 = id == 1 ? 0xFF4F46E5 : 0xFF6B7199; };
  }
  void draw(bool drawdock = true) {
    p.set_opacity(1); p.fill_rrect({0, 0, TW, TH}, 0, Paint::linear(150, 0xFFF1E4FF, 0xFFA99BFF)); p.symbol({400, -30, 120, 250}, 0xFF4F46E5, 0xFF8B5CF6, 0xFFC4B5FD, 0xFFAB8E18);
    if (drawdock) { panel(u, {10, TH - 62, 200, 52}, 20); app_tile(u, {18, TH - 56, 40, 40}, "folder", 0xFF6D66F2, 0xFF4F46E5); app_tile(u, {66, TH - 56, 40, 40}, "settings", 0xFF9AA0C8, 0xFF6B7199); }
    mo.draw(u, [&](uint32_t, BauraVec::Rect b) { list_row(u, {b.x + 10, b.y + 10, b.w - 20, 52}, "folder", "Documents", "12 éléments", State::Normal, "hier", true); list_row(u, {b.x + 10, b.y + 66, b.w - 20, 52}, "image", "Images", "48 éléments"); });
  }
  void run(float seconds) { for (float t = 0; t < seconds - 1e-4f; t += 1.f / 60) mo.update(wm, 1.f / 60); }
};
static void tile(std::vector<uint32_t>& big, int W, const Scene& s, int col, int row) { for (int y = 0; y < TH; ++y) for (int x = 0; x < TW; ++x) big[size_t(row * TH + y) * W + col * TW + x] = s.px[size_t(y) * TW + x]; }
int main() {
  auto blob = read_file("../../common/inter.bgf"); Font f; f.load(blob.data(), blob.size());
  { // --- ouverture : 6 images à 0, 50, 100, 150, 200, 300 ms ---
    Scene s(f); uint32_t id = s.wm.create_window("Fichiers", {70, 40, 380, 230}); std::vector<uint32_t> big(size_t(TW) * 3 * TH * 2); float marks[] = {0, .05f, .10f, .15f, .20f, .30f}; float t = 0;
    s.mo.update(s.wm, 0); for (int i = 0; i < 6; ++i) { s.run(marks[i] - t); t = marks[i]; s.draw(); tile(big, TW * 3, s, i % 3, i / 3); }
    assert(s.mo.find(id)->mode == WindowMotion::Visual::Idle && std::fabs(s.mo.find(id)->scale.pos - 1) < 1e-3f);
    write_ppm("ouverture.ppm", big, TW * 3, TH * 2); }
  { // --- fermeture (3 images) puis réduction (3 images) ---
    Scene s(f); uint32_t a = s.wm.create_window("Fichiers", {70, 40, 380, 230}); s.mo.update(s.wm, 0); s.run(.5f); std::vector<uint32_t> big(size_t(TW) * 3 * TH * 2);
    s.wm.minimize(a); s.mo.update(s.wm, 0); float m[] = {.06f, .13f, .21f}; float t = 0; for (int i = 0; i < 3; ++i) { s.run(m[i] - t); t = m[i]; s.draw(); tile(big, TW * 3, s, i, 0); }
    s.run(.3f); assert(s.mo.find(a)->mode == WindowMotion::Visual::Minimized); s.wm.restore(a); s.mo.update(s.wm, 0);   // restauration depuis la barre
    s.wm.close_window(a); s.mo.update(s.wm, 0); float c[] = {.04f, .09f, .15f}; t = 0; for (int i = 0; i < 3; ++i) { s.run(c[i] - t); t = c[i]; s.draw(); tile(big, TW * 3, s, i, 1); }
    s.run(.3f); assert(s.mo.count() == 0);   // la fenêtre fantôme disparaît après l'animation
    write_ppm("fermeture-reduction.ppm", big, TW * 3, TH * 2); }
  { // --- effet « déplacement » : fenêtre tenue = 97 % + 88 % d'opacité ---
    Scene s(f); uint32_t a = s.wm.create_window("Fichiers", {70, 40, 380, 230}); s.mo.update(s.wm, 0); s.run(.5f); std::vector<uint32_t> big(size_t(TW) * 2 * TH);
    s.draw(); tile(big, TW * 2, s, 0, 0); s.wm.handle_click(150, 60); s.wm.drag_to(200, 90); s.run(.4f); s.draw(); tile(big, TW * 2, s, 1, 0);
    assert(s.mo.find(a)->opacity.pos < .95f); write_ppm("deplacement.ppm", big, TW * 2, TH); }
  { // --- reduce_motion : tout est immédiat ---
    Scene s(f); s.mo.reduce_motion = true; uint32_t a = s.wm.create_window("X", {70, 40, 380, 230}); s.mo.update(s.wm, 0); s.mo.update(s.wm, 0); assert(s.mo.find(a)->mode == WindowMotion::Visual::Idle); }
  { // --- courbes (--spring, --ease-out, ease-in-out, ressort physique) ---
    const int W = 760, H = 300; std::vector<uint32_t> px(size_t(W) * H); BGEFramebuffer fb{px.data(), W, H, W, px.size()}; BGERenderer r(fb); Painter p(r);
    p.fill_rrect({0, 0, W, H}, 0, Paint::linear(150, 0xFFF7F1FF, 0xFFDCD3FF)); f.draw(p, 24, 34, "Courbes de mouvement (temps →, progression ↑)", 14, 0xFF17133D, Weight::SemiBold);
    struct C { const char* n; BauraMotion::Bezier b; uint32_t col; } cs[] = {{"--spring (ouvertures)", SPRING, 0xFF4F46E5}, {"--ease-out (panneaux)", EASE_OUT, 0xFF8B5CF6}, {"ease-in-out (déplacements)", EASE_IN_OUT, 0xFFAB8E18}, {"ease-in (sorties)", EASE_IN, 0xFFD6303C}};
    for (int i = 0; i < 4; ++i) { float ox = 24 + (i % 2) * 360, oy = 56 + (i / 2) * 120; p.fill_rrect({ox, oy, 330, 100}, 14, 0x66FFFFFF); f.draw(p, ox + 10, oy + 18, cs[i].n, 12, 0xFF5F5A88);
      p.fill_rrect({ox + 10, oy + 80 - 54, 310, 1}, 0, 0x33000000); BauraVec::Path pa; pa.subs.push_back({}); for (int k = 0; k <= 60; ++k) { float x = k / 60.f, y = cs[i].b(x); pa.subs.back().pts.push_back({ox + 10 + x * 310, oy + 84 - y * 54}); } p.stroke_path(pa, {0, 0}, 1, 2.4f, cs[i].col); }
    write_ppm("courbes.ppm", px, W, H); }
  std::puts("mouvement : invariants OK");
}
