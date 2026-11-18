// AVANT (dépôt) / APRÈS (charte BAURA) : identité, palette, icônes vectorielles.
#include "baura_text.h"
#include "baura_glass.h"
#include "baura_theme_brand.h"
#include "baura_theme.h"
#include "baura_brand.h"
#include "bmp_out.h"
using namespace BauraVec;
int main(int, char**) {
  const int W = 1100, H = 700; std::vector<uint32_t> px(size_t(W) * H); BGEFramebuffer fb{px.data(), W, H, W, px.size()}; BGERenderer r(fb); Painter p(r);
  auto blob = read_file("../../common/inter.bgf"); Font f; f.load(blob.data(), blob.size());
  // --- colonne AVANT (sombre, dépôt) ---
  r.fill_rect(0, 0, 300, H, BauraTheme::Background); f.draw(p, 20, 36, "AVANT — dépôt actuel", 14, 0xFFB7C2D0, Weight::SemiBold);
  BauraBrand::draw_mark(r, 20, 60, 96); f.draw(p, 20, 176, "Marque : « B » en pixels 5×7", 12, 0xFFB7C2D0);
  uint32_t old[] = {BauraTheme::AccentBlue, BauraTheme::AccentCyan, BauraTheme::AccentPurple, BauraTheme::Success, BauraTheme::Warning, BauraTheme::Danger};
  for (int i = 0; i < 6; ++i) r.draw_rounded_rect(20 + i * 44, 200, 38, 38, 10, old[i]); f.draw(p, 20, 262, "Palette bleu / cyan (≠ charte)", 12, 0xFFB7C2D0);
  const char* ic[] = {"system-search", "network-wireless", "battery", "preferences-system", "window-close", "user-identity"};
  for (int i = 0; i < 6; ++i) r.draw_icon(20 + i * 44, 290, 32, 0xFFF4F7FB, ic[i]); f.draw(p, 20, 352, "Icônes tracées à la main (primitives)", 12, 0xFFB7C2D0);
  // --- APRÈS clair ---
  const BauraBrand::Tokens& L = BauraBrand::LIGHT; const BauraBrand::Tokens& D = BauraBrand::DARK;
  p.fill_rrect({300, 0, 800, 350}, 0, Paint::linear(150, 0xFFF1E4FF, 0xFFA99BFF)); p.fill_rrect({300, 350, 800, 350}, 0, Paint::linear(150, 0xFF1A1450, 0xFF3B1F8F));
  for (int th = 0; th < 2; ++th) {
    const BauraBrand::Tokens& T = th ? D : L; float oy = th ? 350 : 0; uint32_t fg = T.fg, mut = T.mut;
    f.draw(p, 324, oy + 36, th ? "APRÈS — charte BAURA (sombre)" : "APRÈS — charte BAURA (clair)", 14, fg, Weight::SemiBold);
    p.symbol({324, oy + 56, 44, 92}, T.b1, T.b2, T.b3, T.b4);
    uint32_t sw[] = {T.b1, T.b2, T.b3, T.b4}; const char* nm[] = {"indigo", "violet", "lavande", "or"};
    for (int i = 0; i < 4; ++i) { p.fill_rrect({392 + i * 70, oy + 60, 60, 60}, 16, sw[i]); f.draw(p, 392 + i * 70, oy + 140, nm[i], 12, mut); }
    // verre + icônes
    GlassStyle g; g.tint = T.glass; g.line = T.glass_line; g.highlight = T.highlight; Rect pane{324, oy + 170, 750, 160};
    shadow(p, pane, 20, 12, 34, T.shadow1); glass(p, pane, 20, g);
    const char* names[] = {"undo", "redo", "bold", "italic", "underline", "alignL", "listUl", "table", "image", "link", "search", "mail", "star", "trash", "lock", "eye", "play", "volume", "cpu", "monitor", "keyboard", "clock", "hdd", "shield", "zap", "info", "check", "warn"};
    for (int i = 0; i < 28; ++i) p.icon(names[i], {344.f + (i % 14) * 50, oy + 186.f + (i / 14) * 56, 28, 28}, fg);
    f.draw(p, 344, oy + 310, "97 icônes vectorielles (trait 1,9 · bouts ronds) — aucune n'est un emoji", 12, mut);
  }
  write_ppm("identity.ppm", px, W, H); std::puts("ok");
}
