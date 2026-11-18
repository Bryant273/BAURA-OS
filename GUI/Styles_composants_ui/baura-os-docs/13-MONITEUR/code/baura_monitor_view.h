// BAURA OS — baura_monitor_view.h : le MONITEUR. Un gestionnaire des tâches pensé pour tout le monde : on voit d'un coup d'œil
// « tout va bien » ou « quelque chose ralentit l'ordinateur », et on agit avec UN bouton (Terminer). Données : backend Rust (baura_services.hpp).
#ifndef BAURA_MONITOR_VIEW_H
#define BAURA_MONITOR_VIEW_H
#include <cstdio>
#include <string>
#include <vector>
#include "baura_appicons.h"
#include "baura_ui.h"
#include "baura_services.hpp"
namespace BauraMonitor {
using namespace BauraUI;
enum class Page { Overview, Apps, Performance };
inline std::string fmt1(float v) { char b[32]; std::snprintf(b, sizeof b, "%.1f", v); for (char* c = b; *c; ++c) if (*c == '.') *c = ','; return b; }
inline std::string bytes_kib(uint64_t kib) { if (kib >= 1024 * 1024) return fmt1(kib / 1048576.f) + " Go"; if (kib >= 1024) return std::to_string(kib / 1024) + " Mo"; return std::to_string(kib) + " Ko"; }
inline std::string rate(uint64_t bps) { if (bps >= 1000000) return fmt1(bps / 1e6f) + " Mo/s"; if (bps >= 1000) return fmt1(bps / 1e3f) + " Ko/s"; return std::to_string(bps) + " o/s"; }
inline uint32_t impact_color(const Tokens& T, BauraSvc::Impact i) { return i == BauraSvc::Impact::High ? T.err : i == BauraSvc::Impact::Medium ? T.b4 : T.ok; }
inline const char* impact_label(BauraSvc::Impact i) { return i == BauraSvc::Impact::High ? "Élevé" : i == BauraSvc::Impact::Medium ? "Moyen" : "Faible"; }
inline void pill(Ui& u, Rect r, const std::string& s, uint32_t c) { u.p.fill_rrect(r, r.h / 2, u.a(c, .18f)); u.p.fill_rrect({r.x + 9, r.y + r.h / 2 - 3, 6, 6}, 3, c); text_l(u, r.x + 20, r, s, 12, c, Weight::SemiBold); }
// courbe + aire sous la courbe ; max <= 0 : échelle automatique
inline void sparkline(Ui& u, Rect r, const std::vector<float>& v, float mx, uint32_t c) {
  if (v.size() < 2) return;
  if (mx <= 0) { for (float x : v) mx = std::max(mx, x); mx = std::max(mx, 1.f) * 1.15f; }
  std::string line, area; char b[64]; const float step = r.w / float(std::max<size_t>(v.size() - 1, 1));
  for (size_t i = 0; i < v.size(); ++i) { float x = r.x + step * i, y = r.y + r.h - std::min(1.f, v[i] / mx) * r.h; std::snprintf(b, sizeof b, "%c%.1f %.1f", i ? 'L' : 'M', x, y); line += b; }
  std::snprintf(b, sizeof b, "L%.1f %.1fL%.1f %.1fZ", r.x + r.w, r.y + r.h, r.x, r.y + r.h); area = line + b;
  u.p.fill_path(Path::fromSVG(area.c_str(), 8), {0, 0}, 1, Paint::linear(180, u.a(c, .30f), u.a(c, 0.f)));
  u.p.stroke_path(Path::fromSVG(line.c_str(), 8), {0, 0}, 1, 2.2f, Paint(c));
}
// jauge circulaire : pourcentage au centre, arc dégradé (plein cercle = 100 %)
inline void ring(Ui& u, Vec2 c, float rad, float w, float pct, uint32_t c0, uint32_t c1) {
  char b[160]; pct = std::clamp(pct, 0.f, 99.9f); auto arc = [&](float a1) { float a0 = -1.5707963f, a = a0 + a1 * 6.2831853f / 100.f; std::snprintf(b, sizeof b, "M%.2f %.2fA%.2f %.2f 0 %d 1 %.2f %.2f", c.x, c.y - rad, rad, rad, a1 > 50 ? 1 : 0, c.x + rad * std::cos(a), c.y + rad * std::sin(a)); return std::string(b); };
  std::snprintf(b, sizeof b, "M%.2f %.2fA%.2f %.2f 0 1 1 %.2f %.2fA%.2f %.2f 0 1 1 %.2f %.2fZ", c.x, c.y - rad, rad, rad, c.x, c.y + rad, rad, rad, c.x, c.y - rad);
  u.p.stroke_path(Path::fromSVG(b, 24), {0, 0}, 1, w, Paint(u.T.hair));
  if (pct > .5f) u.p.stroke_path(Path::fromSVG(arc(pct).c_str(), 40), {0, 0}, 1, w, Paint::linear(135, c0, c1));
}
struct Card { const char* title; const char* icon; std::string big, sub; float pct; uint32_t c0, c1; std::vector<float> hist; float hist_max; };
inline void stat_card(Ui& u, Rect r, const Card& k) {
  panel(u, r, 22); u.p.icon(k.icon, {r.x + 18, r.y + 16, 18, 18}, u.T.mut, 2.1f); text_l(u, r.x + 42, {r.x, r.y + 14, r.w, 22}, k.title, 13, u.T.mut, Weight::SemiBold);
  ring(u, {r.x + 62, r.y + 104}, 40, 9, k.pct, k.c0, k.c1); text_c(u, {r.x + 22, r.y + 84, 80, 40}, k.big, 17, u.T.fg, Weight::SemiBold);
  u.font.draw(u.p, r.x + 118, r.y + 96, k.sub, 12, u.T.mut); sparkline(u, {r.x + 118, r.y + 108, r.w - 136, 40}, k.hist, k.hist_max, k.c0);
}
// bandeau « verdict » : la phrase que tout le monde comprend
inline void verdict(Ui& u, Rect r, const BauraSvc::Group* worst, float cpu, float mem) {
  bool slow = worst && worst->impact == BauraSvc::Impact::High, tight = mem > 85;
  uint32_t c = (slow || tight) ? u.T.err : u.T.ok; panel(u, r, 22, true); u.p.fill_rrect({r.x + 18, r.y + 18, 44, 44}, 14, Paint::linear(145, u.a(c, .95f), u.a(c, .65f)));
  u.p.icon(slow || tight ? "triangle-alert" : "check", {r.x + 28, r.y + 28, 24, 24}, u.T.on_accent, 2.4f);
  std::string t = tight ? "La mémoire est presque pleine" : slow ? "Une application ralentit votre ordinateur" : "Tout va bien";
  std::string s = tight ? "Fermez des applications que vous n'utilisez pas." : slow ? worst->name + " utilise beaucoup de ressources (" + fmt1(worst->cpu_pct) + " % du processeur, " + bytes_kib(worst->rss_kib) + ")." : "Processeur à " + fmt1(cpu) + " %, mémoire à " + fmt1(mem) + " %. Rien d'inquiétant.";
  u.font.draw(u.p, r.x + 78, r.y + 38, t, 18, u.T.fg, Weight::SemiBold); u.font.draw(u.p, r.x + 78, r.y + 62, u.font.ellipsize(s, r.w - 100, 13), 13, u.T.mut);
}
// une ligne d'application : icône BAURA, nom, nombre de processus, courbe de charge, mémoire, impact, bouton Terminer
inline Rect app_row(Ui& u, Rect r, const BauraSvc::Group& g, bool hot, bool selected, bool expanded, bool compact = false) {
  if (selected) { u.p.fill_rrect(r, 14, u.a(u.T.b1, .12f)); u.p.stroke_rrect(r, 14, 1, u.a(u.T.b1, .35f)); }
  else if (hot) u.p.fill_rrect(r, 14, u.T.hover);
  BauraVec::appicons::Style st; st.shadow = false; BauraVec::appicons::draw(u.p, g.icon.c_str(), {r.x + 12, r.y + 7.f, 40, 40}, st);
  u.font.draw(u.p, r.x + 64, r.y + 24, g.name, 14, u.T.fg, Weight::SemiBold);
  std::string sub = compact ? fmt1(g.cpu_pct) + " % du processeur · " + bytes_kib(g.rss_kib) : std::to_string(g.processes) + " processus · " + std::to_string(g.threads) + " fils";
  u.font.draw(u.p, r.x + 64, r.y + 42, u.font.ellipsize(sub, compact ? r.w - 64 - 250 : 260, 12), 12, u.T.mut);
  float x = r.x + r.w - 470;
  if (!compact) {
    auto col = [&](float dx, const std::string& s) { u.font.draw(u.p, x + dx, r.y + 26, s, 13, u.T.fg, Weight::SemiBold); };
    col(0, fmt1(g.cpu_pct) + " %"); u.font.draw(u.p, x, r.y + 42, "processeur", 11, u.T.mut); col(100, bytes_kib(g.rss_kib)); u.font.draw(u.p, x + 100, r.y + 42, "mémoire", 11, u.T.mut);
  }
  pill(u, {compact ? r.x + r.w - 250 : x + 200, r.y + 14, 92, 26}, impact_label(g.impact), impact_color(u.T, g.impact));
  Rect b{r.x + r.w - 130, r.y + 10, 112, 34};
  if (g.name == "Système") { u.p.icon("lock", {r.x + r.w - 130, r.y + 19, 16, 16}, u.T.mut, 2); u.font.draw(u.p, r.x + r.w - 108, r.y + 32, "Protégé", 12, u.T.mut); return b; }
  Rect bb = button(u, b, "Terminer", expanded ? Kind::Danger : Kind::Secondary);
  if (!expanded) u.p.stroke_rrect(bb, 12, 1, u.a(u.T.b1, .30f));
  return bb;
}
}  // namespace BauraMonitor
#endif
