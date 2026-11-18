// BAURA OS — baura_boot_splash.h : écran de démarrage ANIMÉ, fonction pure du temps (t en secondes, progress 0..1).
// Pas d'état : on peut rendre n'importe quelle image (tests, capture) et rejouer à volonté.
// Séquence (2,4 s) : fond → barre qui pousse (0,10–0,55) → demi-disques qui balaient (0,45–0,95)
//                    → point doré en ressort (0,85–1,25) → mot-symbole + barre de progression (1,1–1,6) → respiration.
#ifndef BAURA_BOOT_SPLASH_H
#define BAURA_BOOT_SPLASH_H
#include "baura_ui.h"
#include "baura_motion.h"
namespace BauraBoot {
using namespace BauraVec; using namespace BauraMotion;
inline float seg(float t, float a, float b) { return t <= a ? 0.f : t >= b ? 1.f : (t - a) / (b - a); }

// reduce_motion : renvoie directement l'état final (logo complet, progression réelle).
inline void draw_splash(BauraUI::Ui& u, int W, int H, float t, float progress, const char* status, bool reduce_motion = false) {
  Painter& p = u.p; const auto& T = u.T; if (reduce_motion) t = 9.f;
  p.fill_rrect({0, 0, float(W), float(H)}, 0, Paint::linear(155, 0xFF120F3A, 0xFF2E1A7A));
  // halo derrière le logo (deux disques doux à très faible alpha : pas de flou coûteux)
  Vec2 c{W / 2.f, H * .44f}; float S = H * .30f / 420.f;                       // échelle du symbole : 30 % de la hauteur
  for (int i = 5; i >= 1; --i) { float r = H * (.10f + i * .055f); p.fill_rrect({c.x - r, c.y - r, 2 * r, 2 * r}, r, with_alpha(T.b2, .035f)); }
  float k_fade = reduce_motion ? 1.f : EASE_OUT(seg(t, 0.f, .25f));
  float bar = EASE_OUT(seg(t, .10f, .55f)), sw = EASE_OUT(seg(t, .45f, .95f)), dot = reduce_motion ? 1.f : SPRING(seg(t, .85f, 1.25f));
  Vec2 pos{c.x - (150 + 199 / 2.f) * S, c.y - (29 + 420 / 2.f) * S};
  static const Path barP = Path::fromSVG("M174 29A24 24 0 0 1 198 53V425A24 24 0 0 1 150 425V53A24 24 0 0 1 174 29Z", 24), top = Path::fromSVG("M248 29A101 102.5 0 0 1 248 234Z", 64),
    bot = Path::fromSVG("M248 245A101 102 0 0 1 248 449Z", 64), dotP = Path::fromSVG("M331 221H334A15 15 0 0 1 349 236V242A15 15 0 0 1 334 257H331A15 15 0 0 1 316 242V236A15 15 0 0 1 331 221Z", 16);
  p.set_opacity(k_fade);
  // barre : pousse depuis le centre (échelle verticale)
  if (bar > 0) { float sy = bar; Path pb = barP; for (auto& s : pb.subs) for (auto& q : s.pts) q.y = 239 + (q.y - 239) * sy;     // 239 = milieu de la barre
    p.fill_path(pb, pos, S, T.b1); }
  // demi-disques : dévoilés de gauche à droite par un clip (bord droit mobile) — mêmes trajets que le logo final
  if (sw > 0) { int x0 = int(pos.x + 198 * S), x1 = int(pos.x + (198 + (349 - 198) * sw) * S) + 1;
    p.set_clip(x0, 0, x1 - x0, H); p.fill_path(top, pos, S, T.b2); p.fill_path(bot, pos, S, T.b3); p.reset_clip(); }
  // point : ressort, il dépasse puis se pose
  if (dot > 0) { float cx = 332.5f, cy = 239.f; Path pd = dotP; for (auto& s : pd.subs) for (auto& q : s.pts) { q.x = cx + (q.x - cx) * dot; q.y = cy + (q.y - cy) * dot; }
    p.fill_path(pd, pos, S, T.b4); }
  p.set_opacity(1.f);
  // mot-symbole + progression
  float k_txt = reduce_motion ? 1.f : EASE_OUT(seg(t, 1.1f, 1.6f)); p.set_opacity(k_txt);
  const char* name = "BAURA OS"; int px = 28; float tw = u.font.measure(name, px, BauraVec::Weight::SemiBold);
  u.font.draw(p, c.x - tw / 2, H * .72f, name, px, 0xFFF4F2FF, BauraVec::Weight::SemiBold);
  float bw = 220, bx = c.x - bw / 2, by = H * .72f + 28;
  p.fill_rrect({bx, by, bw, 6}, 3, 0x26FFFFFF); float pw = std::max(6.f, bw * progress);
  p.fill_rrect({bx, by, pw, 6}, 3, Paint::linear(90, T.b1, T.b2));
  if (status && *status) { int s14 = 14; float sw2 = u.font.measure(status, s14, BauraVec::Weight::Regular); u.font.draw(p, c.x - sw2 / 2, by + 30, status, s14, 0xB3F4F2FF); }
  p.set_opacity(1.f);
}
}  // namespace BauraBoot
#endif
