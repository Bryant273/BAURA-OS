// BAURA OS — baura_native.h : le mode « NATIF ». Ce qui fait qu'un système ressemble à un OS et non à une page web :
// barre de menus, barre d'outils compacte, matières PLEINES pour le contenu (le verre reste réservé au « châssis »), listes en détails
// triables, barre d'état, poignée de redimensionnement, curseurs, mnémoniques de clavier, densité réglable (Paramètres > Apparence).
#ifndef BAURA_NATIVE_H
#define BAURA_NATIVE_H
#include <string>
#include <vector>
#include "baura_appicons.h"
#include "baura_ui.h"
namespace BauraNative {
using namespace BauraUI;
// ---- Densité : la même interface, deux échelles (Confortable = tactile, Compacte = souris/clavier, défaut du bureau) ----
struct Density { float menubar, toolbar, row, header, status, titlebar, sidebar_row, win_radius, ctl_radius, font, font_small; };
inline constexpr Density COMPACT{26, 40, 26, 24, 24, 34, 28, 10, 7, 13, 12};
inline constexpr Density COMFORTABLE{30, 48, 36, 30, 28, 40, 34, 12, 9, 14, 12};
// ---- Matières pleines (opaques) pour le contenu ; le verre n'est utilisé que pour barre des tâches, Démarrer, menus et bulles ----
struct Surface { uint32_t content, sidebar, bar, line, stripe, select_idle; };
inline Surface surface(const Tokens& T, bool dark) { (void)T; return dark ? Surface{0xFF17133A, 0xFF1E1948, 0xFF221C52, 0xFF312A6E, 0xFF1B1741, 0xFF2D2768} : Surface{0xFFFDFCFF, 0xFFF0EDFC, 0xFFEAE6FA, 0xFFD6D0F0, 0xFFF7F5FD, 0xFFE3DEF8}; }
// ---- Curseurs (le compositeur les dessine ; l'application les demande) ----
enum class Cursor { Arrow, Text, Hand, Wait, Move, ResizeNS, ResizeEW, ResizeNWSE, ResizeNESW, NotAllowed };
// ---- Barre de menus avec mnémoniques (la lettre soulignée s'active avec Alt+lettre) ----
struct Menu { std::string label; int mnemonic; };   // mnemonic : index (en octets) de la lettre à souligner
inline float menubar(Ui& u, Rect r, const Surface& S, const std::vector<Menu>& ms, int open, bool alt) {
  u.p.fill_rrect(r, 0, S.bar); u.p.fill_rrect({r.x, r.y + r.h - 1, r.w, 1}, 0, S.line); float x = r.x + 8;
  for (size_t i = 0; i < ms.size(); ++i) { float w = u.font.measure(ms[i].label, 13) + 20; Rect c{x, r.y + 3, w, r.h - 6};
    if (int(i) == open) u.p.fill_rrect(c, 6, u.T.accent);
    uint32_t fg = int(i) == open ? u.T.on_accent : u.T.fg; u.font.draw(u.p, x + 10, r.y + r.h / 2 + 5, ms[i].label, 13, fg);
    if (alt && ms[i].mnemonic >= 0) { float ux = x + 10 + u.font.measure(ms[i].label.substr(0, size_t(ms[i].mnemonic)), 13), uw = u.font.measure(ms[i].label.substr(size_t(ms[i].mnemonic), 1), 13); u.p.fill_rrect({ux, r.y + r.h / 2 + 8, uw, 1}, 0, fg); }
    x += w; }
  return x;
}
// ---- Châssis de fenêtre natif : rayon 10, barre de titre 34 px, ombre courte et nette, contenu PLEIN ----
inline Rect window(Ui& u, Rect r, const Density& D, const Surface& S, const std::string& title, const char* icon, bool focused = true) {
  shadow(u.p, r, D.win_radius, focused ? 14 : 6, focused ? 34 : 16, focused ? 0x55120C3A : 0x30120C3A); u.p.fill_rrect(r, D.win_radius, S.content); u.p.stroke_rrect(r, D.win_radius, 1, focused ? S.line : u.a(S.line, .6f));
  u.p.set_clip(int(r.x), int(r.y), int(r.w), int(D.titlebar)); u.p.fill_rrect({r.x, r.y, r.w, D.titlebar + D.win_radius}, D.win_radius, S.bar); u.p.reset_clip();
  u.p.icon(icon, {r.x + 12, r.y + (D.titlebar - 16) / 2, 16, 16}, focused ? u.T.accent : u.T.mut, 2); u.font.draw(u.p, r.x + 36, r.y + D.titlebar / 2 + 5, title, 13, focused ? u.T.fg : u.T.mut, Weight::SemiBold);
  const char* ic[3] = {"x", "square", "minus"}; for (int i = 0; i < 3; ++i) { Rect b{r.x + r.w - 46.f * (i + 1), r.y, 46, D.titlebar}; if (i == 0 && focused) { u.p.set_clip(int(b.x), int(b.y), int(b.w), int(b.h)); u.p.fill_rrect({b.x - 12, b.y, b.w + 12, b.h}, D.win_radius, u.a(u.T.err, .0f)); u.p.reset_clip(); } u.p.icon(ic[i], {b.x + 16, b.y + (D.titlebar - 14) / 2, 14, 14}, u.T.fg, 1.8f); }
  u.p.fill_rrect({r.x, r.y + D.titlebar - 1, r.w, 1}, 0, S.line); return {r.x, r.y + D.titlebar, r.w, r.h - D.titlebar};
}
// ---- Barre d'outils : boutons icône 30×28 SANS cadre (cadre seulement au survol), champ de chemin, recherche ----
inline void tool_button(Ui& u, Rect r, const char* icon, bool enabled = true, bool hot = false, bool on = false) { if (hot || on) u.p.fill_rrect(r, 6, on ? u.a(u.T.accent, .2f) : u.T.hover); u.p.icon(icon, {r.x + 6, r.y + 5, 18, 18}, enabled ? (on ? u.T.accent : u.T.fg) : u.a(u.T.mut, .5f), 1.9f); }
inline void field(Ui& u, Rect r, const Surface& S, const Density& D) { u.p.fill_rrect(r, D.ctl_radius, S.content); u.p.stroke_rrect(r, D.ctl_radius, 1, S.line); }
// ---- Barre latérale : sections repliables, lignes 28 px, icônes 18 px, sélection pleine accent ----
struct SideItem { const char* icon; std::string label; bool header; bool selected; };
inline void sidebar(Ui& u, Rect r, const Surface& S, const Density& D, const std::vector<SideItem>& items) {
  u.p.fill_rrect(r, 0, S.sidebar); u.p.fill_rrect({r.x + r.w - 1, r.y, 1, r.h}, 0, S.line); float y = r.y + 8;
  for (auto& it : items) {
    if (it.header) { y += 6; u.font.draw(u.p, r.x + 14, y + 14, it.label, 11, u.T.mut, Weight::SemiBold); y += 22; continue; }
    Rect row{r.x + 8, y, r.w - 16, D.sidebar_row - 2}; if (it.selected) u.p.fill_rrect(row, 6, u.T.accent);
    BauraVec::appicons::Style st; st.shadow = false; if (it.icon[0] == '@') BauraVec::appicons::draw(u.p, it.icon + 1, {row.x + 6, row.y + 3, row.h - 6, row.h - 6}, st); else u.p.icon(it.icon, {row.x + 8, row.y + 5, 17, 17}, it.selected ? u.T.on_accent : u.T.mut, 1.9f);
    u.font.draw(u.p, row.x + 34, row.y + row.h / 2 + 5, it.label, 13, it.selected ? u.T.on_accent : u.T.fg); y += D.sidebar_row; }
}
// ---- Liste en détails : en-têtes cliquables (flèche de tri), lignes zébrées, sélection accent, rectangle de focus clavier ----
struct Col { std::string title; float w; bool right; };
struct Row { const char* icon; std::vector<std::string> cells; };
inline void details(Ui& u, Rect r, const Surface& S, const Density& D, const std::vector<Col>& cols, const std::vector<Row>& rows, int sort_col, bool asc, int selected, int focus, int hover = -1) {
  u.p.fill_rrect({r.x, r.y, r.w, D.header}, 0, S.content); u.p.fill_rrect({r.x, r.y + D.header - 1, r.w, 1}, 0, S.line); float x = r.x + 12;
  for (size_t c = 0; c < cols.size(); ++c) { float w = c == 0 ? r.w - 12 - 3 * 100 : cols[c].w; u.font.draw(u.p, x + (c == 0 ? 26 : 8), r.y + D.header / 2 + 4, cols[c].title, 12, u.T.fg, Weight::SemiBold);
    if (int(c) == sort_col) u.p.icon(asc ? "chevron-up" : "chevron-down", {x + (c == 0 ? 26 : 8) + u.font.measure(cols[c].title, 12, Weight::SemiBold) + 4, r.y + (D.header - 12) / 2, 12, 12}, u.T.mut, 2.2f);
    if (c) u.p.fill_rrect({x, r.y + 5, 1, D.header - 10}, 0, S.line);
    x += w;
  }
  u.p.set_clip(int(r.x), int(r.y + D.header), int(r.w), int(r.h - D.header)); float y = r.y + D.header;
  for (size_t i = 0; i < rows.size(); ++i, y += D.row) { Rect rr{r.x, y, r.w, D.row}; bool sel = int(i) == selected;
    if (sel) u.p.fill_rrect(rr, 0, u.T.accent); else if (int(i) == hover) u.p.fill_rrect(rr, 0, S.select_idle); else if (i % 2) u.p.fill_rrect(rr, 0, S.stripe);
    uint32_t fg = sel ? u.T.on_accent : u.T.fg; float cx = r.x + 12;
    for (size_t c = 0; c < cols.size() && c < rows[i].cells.size(); ++c) { float w = c == 0 ? r.w - 12 - 3 * 100 : cols[c].w; float tx = cx + (c == 0 ? 26 : 8);
      if (c == 0) { BauraVec::appicons::Style st; st.shadow = false; BauraVec::appicons::draw(u.p, rows[i].icon, {cx, y + (D.row - 20) / 2, 20, 20}, st); }
      std::string t = u.font.ellipsize(rows[i].cells[c], w - (c == 0 ? 34 : 16), 13); float tw = u.font.measure(t, 13); u.font.draw(u.p, cols[c].right ? cx + w - 10 - tw : tx, y + D.row / 2 + 5, t, 13, c == 0 || sel ? fg : (sel ? fg : u.T.mut)); cx += w; }
    if (int(i) == focus) { u.p.stroke_rrect({rr.x + 1, rr.y + 1, rr.w - 2, rr.h - 2}, 0, 1, sel ? u.T.on_accent : u.T.accent); } }
  u.p.reset_clip();
}
inline void statusbar(Ui& u, Rect r, const Surface& S, const std::string& left, const std::string& right) {
  u.p.fill_rrect(r, 0, S.bar); u.p.fill_rrect({r.x, r.y, r.w, 1}, 0, S.line); u.font.draw(u.p, r.x + 12, r.y + r.h / 2 + 4, left, 12, u.T.mut); u.font.draw(u.p, r.x + r.w - 30 - u.font.measure(right, 12), r.y + r.h / 2 + 4, right, 12, u.T.mut);
}
// poignée de redimensionnement (coin bas-droit) : 3 diagonales, zone cliquable 16×16 qui demande le curseur ResizeNWSE
inline Rect grip(Ui& u, Rect win) { for (int i = 0; i < 3; ++i) u.p.stroke_path(Path::fromSVG(("M" + std::to_string(int(win.x + win.w - 5 - i * 4)) + " " + std::to_string(int(win.y + win.h - 3)) + "L" + std::to_string(int(win.x + win.w - 3)) + " " + std::to_string(int(win.y + win.h - 5 - i * 4))).c_str(), 2), {0, 0}, 1, 1.4f, Paint(u.a(u.T.mut, .8f))); return {win.x + win.w - 16, win.y + win.h - 16, 16, 16}; }
// ---- Menu contextuel : lignes 26 px, raccourcis alignés à droite, séparateurs, élément désactivé ----
struct Item { std::string label, shortcut; bool sep, enabled, hot; };
inline Rect context_menu(Ui& u, Vec2 pos, const Surface& S, const std::vector<Item>& items) {
  float w = 230, h = 8; for (auto& i : items) h += i.sep ? 9.f : 26.f; Rect r{pos.x, pos.y, w, h}; panel(u, r, 10, true); float y = r.y + 4;
  for (auto& i : items) { if (i.sep) { u.p.fill_rrect({r.x + 10, y + 4, r.w - 20, 1}, 0, S.line); y += 9; continue; } Rect row{r.x + 4, y, r.w - 8, 26}; if (i.hot) u.p.fill_rrect(row, 6, u.T.accent);
    uint32_t fg = !i.enabled ? u.a(u.T.mut, .55f) : i.hot ? u.T.on_accent : u.T.fg; u.font.draw(u.p, row.x + 12, row.y + 18, i.label, 13, fg); u.font.draw(u.p, row.x + row.w - 12 - u.font.measure(i.shortcut, 12), row.y + 18, i.shortcut, 12, i.hot ? u.T.on_accent : u.a(u.T.mut, i.enabled ? 1.f : .55f)); y += 26; }
  return r;
}
}  // namespace BauraNative
#endif
