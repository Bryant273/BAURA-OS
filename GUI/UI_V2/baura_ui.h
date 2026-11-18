// BAURA OS — baura_ui.h : kit de composants (mode « dessin pur » : l'état vient de l'appelant).
// Chaque fonction dessine un composant à partir des JETONS de la charte et retourne sa zone cliquable.
// Les dimensions ici sont la SPÉCIFICATION (04-COMPOSANTS/README.md).
#ifndef BAURA_UI_H
#define BAURA_UI_H
#include "baura_text.h"
#include "baura_glass.h"
#include "baura_theme_brand.h"
#include "baura_appicons.h"
#include <string>
#include <vector>

namespace BauraUI {
using namespace BauraVec;
using Tokens = BauraBrand::Tokens;
enum class State { Normal, Hover, Pressed, Focus, Disabled, Error };
enum class Kind { Primary, Secondary, Ghost, Danger };

struct Ui {
  Painter& p; const Font& font; const Tokens& T; bool rtl = false;
  uint32_t a(uint32_t c, float k) const { return with_alpha(c, k); }
  // rectangle miroir pour l'arabe/RTL : x' = largeur_conteneur - x - w
  Rect mirror(Rect r, float cw) const { return rtl ? Rect{cw - r.x - r.w, r.y, r.w, r.h} : r; }
};
inline uint32_t white(float a = 1) { return with_alpha(0xFFFFFFFF, a); }
inline void text_c(Ui& u, Rect r, const std::string& s, int px, uint32_t c, Weight w = Weight::Regular) {  // centré
  auto m = u.font.metrics(px, w); float tw = u.font.measure(s, px, w); u.font.draw(u.p, r.x + (r.w - tw) / 2, r.y + (r.h + m.ascent - m.descent) / 2 - 1, s, px, c, w);
}
inline void text_l(Ui& u, float x, Rect r, const std::string& s, int px, uint32_t c, Weight w = Weight::Regular) {  // aligné à gauche, centré verticalement
  auto m = u.font.metrics(px, w); u.font.draw(u.p, x, r.y + (r.h + m.ascent - m.descent) / 2 - 1, s, px, c, w);
}
inline void focus_ring(Ui& u, Rect r, float rad) { u.p.stroke_rrect({r.x - 3, r.y - 3, r.w + 6, r.h + 6}, rad + 3, 2, u.a(u.T.b2, .85f)); }

// ---- Boutons : hauteur 36, rayon 12, texte 14/600 ----
inline Rect button(Ui& u, Rect r, const std::string& label, Kind k = Kind::Primary, State s = State::Normal, const char* icon = nullptr) {
  const Tokens& T = u.T; float rad = 12; bool dis = s == State::Disabled, hov = s == State::Hover, prs = s == State::Pressed;
  uint32_t fg = k == Kind::Primary || k == Kind::Danger ? T.on_accent : T.fg;
  if (k == Kind::Primary) { shadow(u.p, r, rad, 6, 14, u.a(T.b1, .35f)); u.p.fill_rrect(r, rad, Paint::linear(145, mix(T.b1, 0xFFFFFFFF, hov ? .12f : 0), mix(T.b2, 0xFFFFFFFF, hov ? .12f : 0))); u.p.stroke_rrect(r, rad, 1, white(.35f)); if (prs) u.p.fill_rrect(r, rad, 0x30000000); }
  else if (k == Kind::Danger) { u.p.fill_rrect(r, rad, Paint::linear(145, mix(T.err, 0xFFFFFFFF, hov ? .12f : 0), mix(T.err, 0xFF000000, .25f))); if (prs) u.p.fill_rrect(r, rad, 0x30000000); }
  else if (k == Kind::Secondary) { u.p.fill_rrect(r, rad, T.field); u.p.stroke_rrect(r, rad, 1, T.field_line); if (hov) u.p.fill_rrect(r, rad, T.hover); if (prs) u.p.fill_rrect(r, rad, T.press); }
  else { if (hov) u.p.fill_rrect(r, rad, T.hover); if (prs) u.p.fill_rrect(r, rad, T.press); }
  if (dis) fg = u.a(fg, .45f);
  float tw = u.font.measure(label, 14, Weight::SemiBold), iw = icon ? 18 : 0, gap = icon ? 8 : 0, x0 = r.x + (r.w - (tw + iw + gap)) / 2;
  if (icon) u.p.icon(icon, {x0, r.y + (r.h - 18) / 2, 18, 18}, fg);
  text_l(u, x0 + iw + gap, r, label, 14, fg, Weight::SemiBold);
  if (s == State::Focus) focus_ring(u, r, rad);
  return r;
}
inline Rect icon_button(Ui& u, Rect r, const char* icon, State s = State::Normal, bool selected = false) {  // 34×32, rayon 12
  if (s == State::Hover) u.p.fill_rrect(r, 12, u.T.hover);
  if (s == State::Pressed || selected) u.p.fill_rrect(r, 12, u.T.press);
  u.p.icon(icon, {r.x + (r.w - 18) / 2, r.y + (r.h - 18) / 2, 18, 18}, s == State::Disabled ? u.a(u.T.fg, .4f) : u.T.fg); if (s == State::Focus) focus_ring(u, r, 12); return r;
}
// ---- Champ de saisie : hauteur 40, rayon 12 ----
inline Rect input(Ui& u, Rect r, const std::string& text, const std::string& placeholder, State s = State::Normal, const char* icon = nullptr, bool password = false, int caret = -1) {
  const Tokens& T = u.T; bool focus = s == State::Focus, err = s == State::Error;
  u.p.fill_rrect(r, 12, s == State::Disabled ? u.a(T.field, .5f) : T.field); u.p.stroke_rrect(r, 12, focus || err ? 2 : 1, err ? T.err : focus ? T.b2 : (s == State::Hover ? mix(T.field_line, T.b2, .4f) : T.field_line));
  if (focus) shadow(u.p, {r.x, r.y, r.w, r.h}, 12, 0, 12, u.a(T.b2, .25f));
  float x = r.x + 14; if (icon) { u.p.icon(icon, {x, r.y + (r.h - 18) / 2, 18, 18}, T.mut); x += 28; }
  std::string shown = password ? std::string(text.size() ? u.font.measure(text, 14) > 0 ? text.size() : 0 : 0, '*') : text;
  if (password) { shown.clear(); for (size_t i = 0; i < text.size(); ++i) u.p.fill_rrect({x + i * 12.f, r.y + r.h / 2 - 4, 8, 8}, 4, T.fg); }
  else if (text.empty()) text_l(u, x, r, placeholder, 14, T.mut); else text_l(u, x, r, u.font.ellipsize(text, r.w - (x - r.x) - 14, 14), 14, T.fg);
  if (focus && caret >= 0) { float cx = x + (password ? caret * 12.f : u.font.measure(text.substr(0, size_t(caret)), 14)); u.p.fill_rrect({cx, r.y + 10, 1.6f, r.h - 20}, .8f, T.fg); }
  return r;
}
// ---- Contrôles ----
inline void checkbox(Ui& u, Rect r, bool on, State s = State::Normal) {  // 20×20, rayon 7
  r.w = r.h = 20; if (on) { u.p.fill_rrect(r, 7, Paint::linear(145, u.T.b1, u.T.b2)); u.p.icon("check", r.inset(2), 0xFFFFFFFF, 2.6f); } else { u.p.fill_rrect(r, 7, u.T.field); u.p.stroke_rrect(r, 7, 1.5f, s == State::Hover ? u.T.b2 : u.T.field_line); }
  if (s == State::Focus) focus_ring(u, r, 7);
}
inline void radio(Ui& u, Rect r, bool on, State s = State::Normal) {
  r.w = r.h = 20; u.p.fill_rrect(r, 10, on ? Paint(u.T.b1) : Paint(u.T.field)); u.p.stroke_rrect(r, 10, 1.5f, on ? u.T.b1 : u.T.field_line); if (on) u.p.fill_rrect({r.x + 6, r.y + 6, 8, 8}, 4, 0xFFFFFFFF); if (s == State::Focus) focus_ring(u, r, 10);
}
inline void toggle(Ui& u, Rect r, bool on, float t = -1) {  // 44×26 ; t = progression d'animation 0..1
  r.w = 44; r.h = 26; if (t < 0) t = on ? 1.f : 0.f; u.p.fill_rrect(r, 13, Paint::linear(90, mix(u.T.hair, u.T.b1, t), mix(u.T.hair, u.T.b2, t))); if (t < .5f) u.p.stroke_rrect(r, 13, 1, u.T.field_line);
  float x = r.x + 3 + 18 * t; shadow(u.p, {x, r.y + 3, 20, 20}, 10, 1, 4, 0x55000000); u.p.fill_rrect({x, r.y + 3, 20, 20}, 10, 0xFFFFFFFF);
}
inline void slider(Ui& u, Rect r, float v) {  // piste 6 px, poignée 20 px
  float cy = r.y + r.h / 2; u.p.fill_rrect({r.x, cy - 3, r.w, 6}, 3, u.T.hair); u.p.fill_rrect({r.x, cy - 3, r.w * v, 6}, 3, Paint::linear(90, u.T.b1, u.T.b2));
  float x = r.x + r.w * v - 10; shadow(u.p, {x, cy - 10, 20, 20}, 10, 2, 6, 0x55000000); u.p.fill_rrect({x, cy - 10, 20, 20}, 10, 0xFFFFFFFF); u.p.stroke_rrect({x, cy - 10, 20, 20}, 10, 2, u.T.b1);
}
inline void progress(Ui& u, Rect r, float v) { u.p.fill_rrect({r.x, r.y, r.w, 8}, 4, u.T.hair); u.p.fill_rrect({r.x, r.y, std::max(8.f, r.w * v), 8}, 4, Paint::linear(90, u.T.b1, u.T.b2)); }
inline void spinner(Ui& u, Vec2 c, float rad, float phase) {  // arc de 270°, phase en tours
  for (int i = 0; i < 24; ++i) { float a = (phase + i / 24.f * .75f) * 6.2831853f, k = i / 23.f; u.p.fill_rrect({c.x + std::cos(a) * rad - 2, c.y + std::sin(a) * rad - 2, 4, 4}, 2, u.a(u.T.b2, .15f + .85f * k)); }
}
inline void badge(Ui& u, Rect r, const std::string& s, uint32_t bg) { u.p.fill_rrect(r, r.h / 2, bg); text_c(u, r, s, 12, 0xFFFFFFFF, Weight::SemiBold); }
inline void chip(Ui& u, Rect r, const std::string& s, bool on) { u.p.fill_rrect(r, r.h / 2, on ? u.a(u.T.b1, .16f) : u.T.field); u.p.stroke_rrect(r, r.h / 2, 1, on ? u.T.b1 : u.T.field_line); text_c(u, r, s, 13, on ? u.T.accent : u.T.fg, Weight::SemiBold); }
// ---- Onglets de fenêtre (style Windows 11) : hauteur 36, rayon 11 11 0 0 ----
struct Tab { std::string title; const char* icon; bool active; };
inline void tabs(Ui& u, Rect r, const std::vector<Tab>& ts) {
  float x = r.x; for (auto& t : ts) { Rect tr{x, r.y + 4, 176, r.h - 4}; if (t.active) { u.p.set_clip(int(r.x), int(r.y), int(r.w), int(r.h)); u.p.fill_rrect({tr.x, tr.y, tr.w, tr.h + 14}, 11, u.T.win); u.p.reset_clip(); }
    u.p.icon(t.icon, {tr.x + 12, tr.y + (tr.h - 16) / 2, 16, 16}, t.active ? u.T.accent : u.T.mut); text_l(u, tr.x + 36, tr, u.font.ellipsize(t.title, 100, 13), 13, t.active ? u.T.fg : u.T.mut, t.active ? Weight::SemiBold : Weight::Regular);
    u.p.icon("x", {tr.x + tr.w - 26, tr.y + (tr.h - 14) / 2, 14, 14}, u.T.mut, 2.2f); x += 180; }
  u.p.icon("plus", {x + 4, r.y + 10, 18, 18}, u.T.mut);
}
struct Seg { std::string label; bool on; };
inline void segmented(Ui& u, Rect r, const std::vector<Seg>& ss) {  // hauteur 34
  u.p.fill_rrect(r, 12, u.T.field); u.p.stroke_rrect(r, 12, 1, u.T.field_line); float w = r.w / ss.size();
  for (size_t i = 0; i < ss.size(); ++i) { Rect c{r.x + i * w + 3, r.y + 3, w - 6, r.h - 6}; if (ss[i].on) { shadow(u.p, c, 9, 2, 6, 0x22000000); u.p.fill_rrect(c, 9, Paint::linear(145, u.T.b1, u.T.b2)); } text_c(u, c, ss[i].label, 13, ss[i].on ? 0xFFFFFFFF : u.T.fg, Weight::SemiBold); }
}
// ---- Ligne de liste : hauteur 52 ----
inline void list_row(Ui& u, Rect r, const char* icon, const std::string& title, const std::string& sub, State s = State::Normal, const std::string& trailing = "", bool selected = false) {
  if (selected) { u.p.fill_rrect(r, 14, u.a(u.T.b1, .14f)); u.p.fill_rrect({r.x, r.y + 12, 3, r.h - 24}, 1.5f, u.T.b1); } else if (s == State::Hover) u.p.fill_rrect(r, 14, u.T.hover);
  u.p.fill_rrect({r.x + 10, r.y + 8, 36, 36}, 11, selected ? Paint::linear(145, u.T.b1, u.T.b2) : Paint(u.T.hair)); u.p.icon(icon, {r.x + 17, r.y + 15, 22, 22}, selected ? 0xFFFFFFFF : u.T.fg);
  text_l(u, r.x + 58, {r.x, r.y, r.w, r.h / 2 + 4}, title, 14, u.T.fg, Weight::SemiBold); if (!sub.empty()) text_l(u, r.x + 58, {r.x, r.y + r.h / 2 - 4, r.w, r.h / 2 + 4}, sub, 12, u.T.mut);
  if (!trailing.empty()) { float w = u.font.measure(trailing, 12); text_l(u, r.x + r.w - w - 14, r, trailing, 12, u.T.mut); }
}
// ---- Menu contextuel : verre fort, rayon 16, éléments 34 px ----
struct MenuItem { std::string label; const char* icon; std::string kbd; bool sep = false, disabled = false, checked = false, sub = false, danger = false; };
inline Rect menu(Ui& u, Vec2 pos, float w, const std::vector<MenuItem>& items, int hot = -1) {
  float h = 12; for (auto& i : items) h += i.sep ? 9 : 34; Rect r{pos.x, pos.y, w, h};
  GlassStyle g; g.tint = u.T.glass_strong; g.line = u.T.glass_line; g.highlight = u.T.highlight; shadow(u.p, r, 16, 14, 34, u.T.shadow1); glass(u.p, r, 16, g);
  float y = r.y + 6; for (size_t n = 0; n < items.size(); ++n) { auto& i = items[n];
    if (i.sep) { u.p.fill_rrect({r.x + 14, y + 4, r.w - 28, 1}, 0, u.T.hair); y += 9; continue; }
    Rect row{r.x + 6, y, r.w - 12, 34}; if (int(n) == hot && !i.disabled) u.p.fill_rrect(row, 10, u.T.hover);
    uint32_t c = i.disabled ? u.a(u.T.fg, .4f) : i.danger ? u.T.err : u.T.fg; if (i.icon) u.p.icon(i.icon, {row.x + 10, row.y + 8, 18, 18}, c); if (i.checked) u.p.icon("check", {row.x + 10, row.y + 8, 18, 18}, u.T.accent, 2.4f);
    text_l(u, row.x + 38, row, i.label, 14, c); if (!i.kbd.empty()) { float kw = u.font.measure(i.kbd, 12); text_l(u, row.x + row.w - kw - 12, row, i.kbd, 12, u.T.mut, Weight::SemiBold); } if (i.sub) u.p.icon("chevron-right", {row.x + row.w - 28, row.y + 8, 18, 18}, u.T.mut);
    y += 34; }
  return r;
}
inline void tooltip(Ui& u, Vec2 pos, const std::string& s) {  // bulle inversée : fond = couleur du texte
  float w = u.font.measure(s, 12, Weight::SemiBold) + 20; Rect r{pos.x, pos.y, w, 28}; u.p.fill_rrect(r, 9, u.T.fg);
  text_c(u, r, s, 12, u.T.fg == BauraBrand::LIGHT.fg ? 0xFFFFFFFF : 0xFF17133D, Weight::SemiBold);
}
inline void scrollbar(Ui& u, Rect track, float start, float len) { u.p.fill_rrect({track.x + track.w - 6, track.y + 4 + (track.h - 8) * start, 4, (track.h - 8) * len}, 2, u.a(u.T.mut, .5f)); }
// ---- Surface de verre standard (carte / panneau) ----
inline void panel(Ui& u, Rect r, float rad, bool strong = false) { GlassStyle g; g.tint = strong ? u.T.glass_strong : u.T.glass; g.line = u.T.glass_line; g.highlight = u.T.highlight; shadow(u.p, r, rad, 12, 34, u.T.shadow1); shadow(u.p, r, rad, 2, 6, u.T.shadow2); glass(u.p, r, rad, g); }
// ---- Tuile d'application : dégradé 145°, bord blanc, ombre ----
// Style des icônes d'applications : true = icônes BAURA dessinées (doc 11, défaut) ; false = « icônes d'origine » (tuile + glyphe de trait).
inline bool& baura_icons_enabled() { static bool on = true; return on; }
// Ancien nom de glyphe (Lucide) → icône BAURA de l'application correspondante.
inline const char* baura_icon_for(const char* glyph) {
  static const char* const M[][2] = {{"folder", "files"}, {"globe", "houb"}, {"music", "media"}, {"shopping-bag", "store"}, {"volume-2", "sound"}, {"shield-check", "security"}, {"refresh-cw", "update"}, {"battery-charging", "power"}, {"activity", "monitor"}, {"image", "photos"}, {"file-text", "file-text"}, {"film", "video"}, {"mail", "mail"}, {"calendar", "calendar"}};
  for (auto& m : M) if (!std::strcmp(m[0], glyph)) return m[1];
  return glyph;
}
inline bool baura_icon_draw(Painter& p, const char* glyph, Rect r, bool shadow = true) {
  if (!baura_icons_enabled()) return false;
  const char* id = baura_icon_for(glyph); if (!BauraVec::appicons::find(id)) return false;
  BauraVec::appicons::Style st; st.shadow = shadow; return BauraVec::appicons::draw(p, id, r, st);
}
// ---- Tuile d'application : icône BAURA (tuile dégradée + motif + point doré) ; repli : dégradé 145° + glyphe ----
inline void app_tile(Ui& u, Rect r, const char* icon, uint32_t c1, uint32_t c2) {
  if (baura_icon_draw(u.p, icon, {r.x - r.w * .0625f, r.y - r.h * .0625f, r.w * 1.125f, r.h * 1.125f})) return;   // la tuile occupe 56/64 de la grille : on agrandit pour garder l'emprise
  float rad = r.w * .32f; shadow(u.p, r, rad, 4, 10, 0x48281E70); u.p.fill_rrect(r, rad, Paint::linear(145, c1, c2)); u.p.stroke_rrect(r, rad, 1, white(.4f)); u.p.icon(icon, r.inset(r.w * .24f), 0xFFFFFFFF);
}
// ---- Fenêtre : cadre verre, barre 46 px, boutons 32×24 ----
inline Rect window_frame(Ui& u, Rect r, const std::string& title, const char* icon, uint32_t c1, uint32_t c2, bool focused = true, bool admin = false) {
  const Tokens& T = u.T; float rad = 22, bar = 46; shadow(u.p, r, rad, 30, 70, focused ? 0x59140D40 : 0x33140D40); shadow(u.p, r, rad, 12, 34, T.shadow1);
  GlassStyle g; g.tint = T.win; g.line = T.win_line; g.highlight = T.highlight; glass(u.p, r, rad, g);
  u.p.set_clip(int(r.x), int(r.y), int(r.w), int(bar)); u.p.fill_rrect({r.x, r.y, r.w, bar + rad}, rad, T.win_bar); u.p.reset_clip(); u.p.fill_rrect({r.x, r.y + bar - 1, r.w, 1}, 0, T.hair);
  Rect ic{r.x + 14, r.y + 12, 22, 22}; if (!baura_icon_draw(u.p, icon, {ic.x - 2, ic.y - 2, 26, 26}, false)) { u.p.fill_rrect(ic, 7, Paint::linear(145, c1, c2)); u.p.icon(icon, ic.inset(4), 0xFFFFFFFF); }
  u.font.draw(u.p, r.x + 46, r.y + 29, title, 14, focused ? T.fg : T.mut, Weight::SemiBold);
  if (admin) { float tw = u.font.measure(title, 14, Weight::SemiBold); Rect b{r.x + 46 + tw + 10, r.y + 14, 86, 18}; u.p.fill_rrect(b, 9, u.a(T.b4, .22f)); u.p.icon("shield", {b.x + 7, b.y + 2, 14, 14}, T.b4, 2.2f); u.font.draw(u.p, b.x + 25, b.y + 13, "Administrateur", 11, T.b4, Weight::SemiBold); }
  float bx = r.x + r.w - 14; const char* ic3[3] = {"x", "square", "minus"}; uint32_t bg[3] = {u.a(T.b4, .22f), u.a(T.b3, .35f), u.a(T.b3, .35f)};
  for (int i = 0; i < 3; ++i) { Rect b{bx - 32 - i * 38.f, r.y + 11, 32, 24}; u.p.fill_rrect(b, 9, bg[i]); u.p.icon(ic3[i], {b.x + 9, b.y + 5, 14, 14}, T.fg, 2.2f); }
  return {r.x, r.y + bar, r.w, r.h - bar};
}
// ---- Notification (toast) : 332 px, rayon 20 ----
inline void toast(Ui& u, Rect r, const char* icon, uint32_t c1, uint32_t c2, const std::string& title, const std::string& text) {
  panel(u, r, 20, true); app_tile(u, {r.x + 14, r.y + 13, 40, 40}, icon, c1, c2); u.font.draw(u.p, r.x + 66, r.y + 30, title, 14, u.T.fg, Weight::SemiBold);
  u.font.draw(u.p, r.x + 66, r.y + 50, u.font.ellipsize(text, r.w - 82, 13), 13, u.T.mut);
}
}  // namespace BauraUI
#endif
