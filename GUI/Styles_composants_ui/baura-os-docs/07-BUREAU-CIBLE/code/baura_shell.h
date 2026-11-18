// BAURA OS — baura_shell.h : les écrans de la COQUE (bureau, barre, menu démarrer, recherche, centre d'actions,
// connexion, installateur, UAC, bulle système) tels qu'ils doivent être. Dessin pur : l'état vient de l'appelant.
// Chaque fonction correspond à un écran du dépôt actuel (tableau « actuel → cible » dans le README).
#ifndef BAURA_SHELL_H
#define BAURA_SHELL_H
#include "baura_ui.h"
namespace BauraShell {
using namespace BauraUI;

struct AppDef { const char* id; const char* label; const char* icon; uint32_t c1, c2; };
inline const AppDef APPS[] = {
  {"files", "Fichiers", "folder", 0xFF6D66F2, 0xFF4F46E5}, {"settings", "Paramètres", "settings", 0xFF9AA0C8, 0xFF6B7199},
  {"terminal", "Terminal", "terminal", 0xFF34D399, 0xFF047857}, {"tasks", "Tâches", "activity", 0xFFF87171, 0xFFB91C1C},
  {"store", "Boutique", "shopping-bag", 0xFFFB923C, 0xFFEA580C}, {"houb", "HOUB", "globe", 0xFF38BDF8, 0xFF0369A1},
  {"media", "BMedia", "music", 0xFFF472B6, 0xFFBE185D}, {"sound", "Son", "volume-2", 0xFFA78BFA, 0xFF6D28D9},
  {"security", "Sécurité", "shield-check", 0xFF34D399, 0xFF0F766E}, {"update", "Mise à jour", "refresh-cw", 0xFF60A5FA, 0xFF1D4ED8},
  {"power", "Énergie", "battery-charging", 0xFFFBBF24, 0xFFD97706}, {"network", "Réseau", "wifi", 0xFF60A5FA, 0xFF2563EB}};
inline const AppDef& app(const char* id) { for (auto& a : APPS) if (!std::strcmp(a.id, id)) return a; return APPS[0]; }

// ---- Fond d'écran : dégradé + masses douces + symbole géant très discret ----
inline void wallpaper(Ui& u, int W, int H, bool dark) {
  Painter& p = u.p; const auto& T = u.T;
  p.fill_rrect({0, 0, float(W), float(H)}, 0, dark ? Paint::linear(150, 0xFF1A1450, 0xFF3B1F8F) : Paint::linear(150, 0xFFF1E4FF, 0xFFA99BFF));
  for (int i = 6; i >= 1; --i) { float r = H * (.16f + i * .07f); p.fill_rrect({W * .22f - r, H * .30f - r, 2 * r, 2 * r}, r, with_alpha(dark ? T.b2 : 0xFFFFFFFF, dark ? .030f : .05f)); }
  for (int i = 5; i >= 1; --i) { float r = H * (.12f + i * .06f); p.fill_rrect({W * .82f - r, H * .78f - r, 2 * r, 2 * r}, r, with_alpha(dark ? T.b1 : T.b3, dark ? .05f : .07f)); }
  float k = dark ? .13f : .16f; p.symbol({W * .62f, H * .06f, W * .30f, H * .74f}, with_alpha(T.b1, k), with_alpha(T.b2, k), with_alpha(T.b3, k), with_alpha(T.b4, k));
}
inline void label_c(Ui& u, Rect r, const std::string& s, int px, uint32_t c, Weight w = Weight::Regular) { text_c(u, r, s, px, c, w); }

// ---- Barre des tâches FLOTTANTE (pilule de verre, centrée, 14 px du bord) ----
// Contenu : bouton Démarrer · champ de recherche · applications (point = ouverte, barre = active) · séparateur · corbeille.
struct TaskbarState { const char* pinned[8]; int n = 0; int active = -1; int open_mask = 0; int hover = -1; };
inline Rect taskbar(Ui& u, int W, int H, const TaskbarState& s, bool start_open = false) {
  float bw = 18 + 44 + 8 + 190 + 14 + s.n * 48 + 14 + 44 + 14; Rect r{(W - bw) / 2, H - 14.f - 60, bw, 60};
  panel(u, r, 30, true); float x = r.x + 12;
  Rect st{x, r.y + 8, 44, 44}; if (start_open) u.p.fill_rrect(st, 14, u.a(u.T.b1, .18f));
  u.p.symbol({st.x + 9, st.y + 5, 26, 34}, u.T.b1, u.T.b2, u.T.b3, u.T.b4); x += 52;
  Rect sr{x, r.y + 12, 190, 36}; u.p.fill_rrect(sr, 18, u.T.field); u.p.stroke_rrect(sr, 18, 1, u.T.field_line); u.p.icon("search", {sr.x + 12, sr.y + 9, 18, 18}, u.T.mut); text_l(u, sr.x + 38, sr, "Rechercher", 13, u.T.mut); x += 204;
  for (int i = 0; i < s.n; ++i) { const AppDef& a = app(s.pinned[i]); Rect t{x, r.y + 9, 42, 42};
    if (i == s.hover) { u.p.fill_rrect({t.x - 3, t.y - 3, 48, 48}, 15, u.T.hover); }
    app_tile(u, {t.x + 1, t.y + 1 - (i == s.hover ? 3.f : 0.f), 40, 40}, a.icon, a.c1, a.c2);
    if (s.open_mask >> i & 1) u.p.fill_rrect({t.x + (i == s.active ? 10.f : 16.f), r.y + 54, i == s.active ? 22.f : 10.f, 4}, 2, i == s.active ? u.T.b1 : u.T.mut);
    x += 48; }
  u.p.fill_rrect({x, r.y + 16, 1, 28}, 0, u.T.hair); x += 14; u.p.icon("trash-2", {x + 10, r.y + 18, 24, 24}, u.T.fg); return r;
}

// ---- Zone d'état en haut à droite (pilule) : température · réseau · son · batterie · heure/date · notifications ----
inline Rect tray(Ui& u, int W, const char* temp, int notifs, bool notif_open = false) {
  Rect r{W - 14.f - 392, 14, 392, 44}; panel(u, r, 22); float x = r.x + 16;
  u.p.icon("thermometer", {x, r.y + 12, 20, 20}, u.T.b4, 2.1f); u.font.draw(u.p, x + 24, r.y + 27, temp, 13, u.T.fg, Weight::SemiBold); x += 74;
  const char* ic[3] = {"wifi", "volume-2", "battery-medium"}; for (int i = 0; i < 3; ++i) { u.p.icon(ic[i], {x, r.y + 12, 20, 20}, u.T.fg); x += 32; }
  u.p.fill_rrect({x + 2, r.y + 12, 1, 20}, 0, u.T.hair); x += 14;
  u.font.draw(u.p, x, r.y + 20, "12:04", 13, u.T.fg, Weight::SemiBold); u.font.draw(u.p, x, r.y + 35, "mer. 14 oct.", 11, u.T.mut);
  Rect b{r.x + r.w - 46, r.y + 5, 34, 34}; if (notif_open) u.p.fill_rrect(b, 12, u.a(u.T.b1, .18f));
  u.p.icon("bell", {b.x + 7, b.y + 7, 20, 20}, u.T.fg); if (notifs) { u.p.fill_rrect({b.x + 19, b.y + 2, 14, 14}, 7, u.T.err); text_c(u, {b.x + 19, b.y + 2, 14, 14}, std::to_string(notifs), 10, 0xFFFFFFFF, Weight::SemiBold); }
  return r;
}


// =====================================================================================================
//  DISPOSITION PAR DÉFAUT : « Split dock » = la barre du dépôt (BauraTaskbar::draw_taskbar) coupée en DEUX pastilles
//  collées au bord bas — GAUCHE : Démarrer · recherche · applications ; DROITE : réseau · batterie · langue · date/heure —
//  + widgets en haut à droite (média · « + » · température · notifications). Mêmes cotes que le dépôt (barre 58, marge 12).
//  Les deux autres dispositions (barre flottante unifiée, pleine largeur) sont des OPTIONS de personnalisation.
// =====================================================================================================
enum class Layout { SplitDock, FloatingPill, FullWidth };
inline const char* layout_name(Layout l) { return l == Layout::SplitDock ? "Barre en deux parties" : l == Layout::FloatingPill ? "Barre flottante" : "Barre pleine largeur"; }
struct StatusState { bool net = true, battery = true, charging = false; int pct = 84; const char* lang = "FR"; const char* time = "12:04"; const char* date = "mer. 14 oct."; };
struct MediaState { bool visible = true, playing = true; const char* title = "Nuit indigo"; const char* artist = "Lumen"; };
struct DockRects { Rect left, right, start, search; };

inline void dock_apps(Ui& u, Rect bar, float x, const TaskbarState& s) {
  for (int i = 0; i < s.n; ++i) { const AppDef& a = app(s.pinned[i]); Rect t{x, bar.y + (bar.h - 40) / 2, 40, 40}; bool foc = i == s.active;
    if (i == s.hover) u.p.fill_rrect({t.x - 3, t.y - 3, 46, 46}, 14, u.T.hover);
    if (foc) u.p.fill_rrect({t.x - 3, t.y - 3, 46, 46}, 14, u.a(u.T.b1, .16f));
    app_tile(u, {t.x, t.y - (i == s.hover ? 2.f : 0.f), 40, 40}, a.icon, a.c1, a.c2);
    if (s.open_mask >> i & 1) u.p.fill_rrect({t.x + (foc ? 9.f : 15.f), bar.y + bar.h - 7, foc ? 22.f : 10.f, 3}, 1.5f, foc ? u.T.b1 : u.T.mut);
    x += 46;
  }
}
inline void dock_start_search(Ui& u, Rect bar, float& x, bool start_open, Rect& st, Rect& sr, float search_w) {
  st = {x, bar.y + (bar.h - 40) / 2, 40, 40}; if (start_open) u.p.fill_rrect(st, 13, u.a(u.T.b1, .18f)); u.p.symbol({st.x + 8, st.y + 4, 24, 32}, u.T.b1, u.T.b2, u.T.b3, u.T.b4); x += 48;
  sr = {x, bar.y + (bar.h - 34) / 2, search_w, 34}; u.p.fill_rrect(sr, 12, u.T.field); u.p.stroke_rrect(sr, 12, 1, u.T.field_line); u.p.icon("search", {sr.x + 10, sr.y + 8, 18, 18}, u.T.mut); text_l(u, sr.x + 36, sr, "Rechercher", 13, u.T.mut); x += search_w + 12;
}
inline void status_cluster(Ui& u, Rect bar, float x, const StatusState& st) {
  u.p.icon(st.net ? "wifi" : "wifi-off", {x, bar.y + (bar.h - 20) / 2, 20, 20}, u.T.fg); x += 32;
  if (st.battery) { u.p.icon(st.charging ? "battery-charging" : "battery-medium", {x, bar.y + (bar.h - 20) / 2, 20, 20}, st.charging ? u.T.ok : u.T.fg); u.font.draw(u.p, x + 24, bar.y + bar.h / 2 + 5, std::to_string(st.pct) + "%", 12, st.charging ? u.T.ok : u.T.mut, Weight::SemiBold); x += 62; }
  u.font.draw(u.p, x, bar.y + bar.h / 2 + 5, st.lang, 12, u.T.mut, Weight::SemiBold); x += 30; u.p.fill_rrect({x - 8, bar.y + 14, 1, bar.h - 28}, 0, u.T.hair);
  u.font.draw(u.p, x + 6, bar.y + bar.h / 2 - 2, st.time, 14, u.T.fg, Weight::SemiBold); u.font.draw(u.p, x + 6, bar.y + bar.h / 2 + 13, st.date, 11, u.T.mut);
}
inline DockRects dock_split(Ui& u, int W, int H, const TaskbarState& s, const StatusState& st, bool start_open = false) {
  const float bh = 48, y = H - 58.f + 5; DockRects d; float lw = 12 + 40 + 8 + 150 + 12 + s.n * 46 + 6; d.left = {12, y, lw, bh}; float rw = 254; d.right = {W - 12 - rw, y, rw, bh};
  panel(u, d.left, 18, true); panel(u, d.right, 18, true); float x = d.left.x + 10; dock_start_search(u, d.left, x, start_open, d.start, d.search, 150); dock_apps(u, d.left, x, s); status_cluster(u, d.right, d.right.x + 18, st); return d;
}
inline DockRects dock_full(Ui& u, int W, int H, const TaskbarState& s, const StatusState& st, bool start_open = false) {   // option « pleine largeur » : le dock du dépôt tel quel, en une seule barre
  const float bh = 48, y = H - 58.f + 5; DockRects d; d.left = {12, y, W - 24.f, bh}; d.right = d.left; panel(u, d.left, 18, true); float x = d.left.x + 10; dock_start_search(u, d.left, x, start_open, d.start, d.search, 150); dock_apps(u, d.left, x, s); status_cluster(u, d.left, d.left.x + d.left.w - 254 + 18, st); return d;
}
// Widgets du haut : [média] [+] [température] [notifications], alignés à droite, y = 12, hauteur 42 (comme draw_top_widgets du dépôt).
struct TopRects { Rect media, plus, temp, bell; };
inline TopRects top_widgets(Ui& u, int W, const MediaState& m, const char* temp, int notifs, bool notif_open = false) {
  TopRects t; float x = W - 18.f, y = 12; const auto& T = u.T;
  Rect b{x - 34, y + 3, 34, 34}; panel(u, b, 12); if (notif_open) u.p.fill_rrect(b, 12, u.a(T.b1, .2f)); u.p.icon("bell", {b.x + 7, b.y + 7, 20, 20}, T.fg);
  if (notifs) { u.p.fill_rrect({b.x + 22, b.y - 3, 15, 15}, 7.5f, T.err); text_c(u, {b.x + 22, b.y - 3, 15, 15}, std::to_string(notifs), 10, 0xFFFFFFFF, Weight::SemiBold); } t.bell = b; x = b.x - 8;
  Rect tp{x - 76, y + 3, 76, 34}; panel(u, tp, 12); u.p.icon("thermometer", {tp.x + 10, tp.y + 7, 20, 20}, T.b4, 2.1f); u.font.draw(u.p, tp.x + 34, tp.y + 22, temp, 13, T.fg, Weight::SemiBold); t.temp = tp; x = tp.x - 8;
  Rect pl{x - 34, y + 3, 34, 34}; panel(u, pl, 12); u.p.icon("plus", {pl.x + 7, pl.y + 7, 20, 20}, T.accent, 2.2f); t.plus = pl; x = pl.x - 8;
  if (m.visible) { Rect md{x - 220, y, 220, 42}; panel(u, md, 14); u.p.fill_rrect({md.x + 8, md.y + 6, 30, 30}, 10, Paint::linear(145, 0xFFF472B6, 0xFFBE185D)); u.p.icon(m.playing ? "pause" : "play", {md.x + 13, md.y + 11, 20, 20}, 0xFFFFFFFF);
    u.font.draw(u.p, md.x + 46, md.y + 18, u.font.ellipsize(m.title, 160, 13, Weight::SemiBold), 13, T.fg, Weight::SemiBold); u.font.draw(u.p, md.x + 46, md.y + 33, u.font.ellipsize(m.artist, 160, 11), 11, T.mut); t.media = md; }
  return t;
}

// ---- Menu Démarrer : recherche · épinglées (grille 6×2) · recommandés · pied (compte, alimentation) ----
inline Rect start_menu(Ui& u, Rect anchor, bool left = false) {
  Rect r{left ? 12.f : anchor.x + anchor.w / 2 - 300, anchor.y - 12 - 540, 600, 540}; panel(u, r, 28, true);
  Rect sf{r.x + 24, r.y + 22, r.w - 48, 42}; u.p.fill_rrect(sf, 21, u.T.field); u.p.stroke_rrect(sf, 21, 1, u.T.field_line); u.p.icon("search", {sf.x + 14, sf.y + 11, 20, 20}, u.T.mut); text_l(u, sf.x + 44, sf, "Rechercher des applications, fichiers, réglages…", 14, u.T.mut);
  u.font.draw(u.p, r.x + 28, r.y + 96, "Épinglées", 14, u.T.fg, Weight::SemiBold); Rect all{r.x + r.w - 130, r.y + 82, 106, 22}; text_l(u, all.x, all, "Toutes les apps", 12, u.T.accent, Weight::SemiBold); u.p.icon("chevron-right", {all.x + 94, all.y + 3, 16, 16}, u.T.accent);
  for (int i = 0; i < 12; ++i) { float cx = r.x + 24 + (i % 6) * 92.f, cy = r.y + 114 + (i / 6) * 100.f; Rect t{cx + 18, cy, 52, 52}; const AppDef& a = APPS[i]; app_tile(u, t, a.icon, a.c1, a.c2); text_c(u, {cx, cy + 58, 88, 20}, a.label, 12, u.T.fg); }
  u.font.draw(u.p, r.x + 28, r.y + 338, "Recommandés", 14, u.T.fg, Weight::SemiBold);
  const char* t[4] = {"Rapport T3.docx", "Vacances.png", "Playlist du soir", "Budget 2026.xlsx"}; const char* sb[4] = {"Il y a 12 min", "Hier", "Lundi", "Samedi"}; const char* ic[4] = {"file-text", "image", "music", "table"};
  for (int i = 0; i < 4; ++i) { Rect row{r.x + 18 + (i % 2) * 282.f, r.y + 356 + (i / 2) * 60.f, 274, 54}; if (i == 1) u.p.fill_rrect(row, 14, u.T.hover);
    u.p.fill_rrect({row.x + 8, row.y + 9, 36, 36}, 11, u.T.hair); u.p.icon(ic[i], {row.x + 15, row.y + 16, 22, 22}, u.T.fg); u.font.draw(u.p, row.x + 56, row.y + 24, t[i], 13, u.T.fg, Weight::SemiBold); u.font.draw(u.p, row.x + 56, row.y + 42, sb[i], 12, u.T.mut); }
  u.p.fill_rrect({r.x + 20, r.y + r.h - 70, r.w - 40, 1}, 0, u.T.hair);
  u.p.fill_rrect({r.x + 24, r.y + r.h - 58, 40, 40}, 20, Paint::linear(145, u.T.b1, u.T.b2)); text_c(u, {r.x + 24, r.y + r.h - 58, 40, 40}, "A", 16, 0xFFFFFFFF, Weight::SemiBold);
  u.font.draw(u.p, r.x + 74, r.y + r.h - 42, "Ahmed", 14, u.T.fg, Weight::SemiBold); u.font.draw(u.p, r.x + 74, r.y + r.h - 25, "Compte administrateur", 12, u.T.mut);
  const char* fi[3] = {"settings", "lock", "power"}; for (int i = 0; i < 3; ++i) { Rect b{r.x + r.w - 24 - 42 * (3 - i) + 6.f, r.y + r.h - 58, 38, 38}; if (i == 2) u.p.fill_rrect(b, 12, u.a(u.T.err, .12f)); u.p.icon(fi[i], {b.x + 9, b.y + 9, 20, 20}, i == 2 ? u.T.err : u.T.fg); }
  return r;
}

// ---- Recherche : champ actif · meilleur résultat · groupes · filtres ----
inline Rect search_panel(Ui& u, Rect anchor, const std::string& q, bool left = false) {
  Rect r{left ? 12.f : anchor.x + anchor.w / 2 - 330, anchor.y - 12 - 500, 660, 500}; panel(u, r, 28, true);
  Rect sf{r.x + 20, r.y + 20, r.w - 40, 46}; u.p.fill_rrect(sf, 23, u.T.field); u.p.stroke_rrect(sf, 23, 2, u.T.b1); u.p.icon("search", {sf.x + 16, sf.y + 12, 22, 22}, u.T.accent);
  float tw = u.font.measure(q, 15); u.font.draw(u.p, sf.x + 48, sf.y + 29, q, 15, u.T.fg); u.p.fill_rrect({sf.x + 49 + tw, sf.y + 12, 2, 22}, 1, u.T.accent);
  const char* ch[5] = {"Tout", "Apps", "Fichiers", "Réglages", "Web"}; float x = r.x + 22; for (int i = 0; i < 5; ++i) { float w = u.font.measure(ch[i], 13, Weight::SemiBold) + 28; chip(u, {x, r.y + 78, w, 28}, ch[i], i == 0); x += w + 8; }
  u.font.draw(u.p, r.x + 28, r.y + 134, "Meilleur résultat", 12, u.T.mut, Weight::SemiBold);
  Rect best{r.x + 16, r.y + 144, r.w - 32, 76}; u.p.fill_rrect(best, 18, u.a(u.T.b1, .14f)); u.p.fill_rrect({best.x, best.y + 16, 3, best.h - 32}, 1.5f, u.T.b1);
  app_tile(u, {best.x + 16, best.y + 12, 52, 52}, "settings", 0xFF9AA0C8, 0xFF6B7199); u.font.draw(u.p, best.x + 84, best.y + 34, "Paramètres", 16, u.T.fg, Weight::SemiBold); u.font.draw(u.p, best.x + 84, best.y + 54, "Application · Entrée pour ouvrir", 12, u.T.mut);
  u.font.draw(u.p, r.x + 28, r.y + 248, "Réglages", 12, u.T.mut, Weight::SemiBold);
  const char* t[3] = {"Paramètres d'affichage", "Paramètres de confidentialité", "Paramètres réseau"}; const char* ic[3] = {"monitor", "lock", "wifi"};
  for (int i = 0; i < 3; ++i) list_row(u, {r.x + 14, r.y + 262 + i * 54.f, r.w - 28, 52}, ic[i], t[i], "Réglage système", i == 1 ? State::Hover : State::Normal);
  u.p.fill_rrect({r.x + 20, r.y + r.h - 44, r.w - 40, 1}, 0, u.T.hair); u.p.icon("corner-down-left", {r.x + 24, r.y + r.h - 32, 16, 16}, u.T.mut); u.font.draw(u.p, r.x + 46, r.y + r.h - 19, "Ouvrir", 12, u.T.mut); u.p.icon("arrow-down", {r.x + 110, r.y + r.h - 32, 16, 16}, u.T.mut); u.font.draw(u.p, r.x + 132, r.y + r.h - 19, "Naviguer", 12, u.T.mut);
  return r;
}

// ---- Centre d'actions : notifications + réglages rapides + médias ----
inline Rect action_center(Ui& u, int W, int H) {
  Rect r{W - 14.f - 392, 62, 392, H - 62.f - 96}; panel(u, r, 28, true); float x = r.x + 18, y = r.y + 18;
  u.font.draw(u.p, x + 4, y + 18, "Notifications", 15, u.T.fg, Weight::SemiBold); text_l(u, r.x + r.w - 110, {r.x, y, 0, 24}, "Tout effacer", 12, u.T.accent, Weight::SemiBold); y += 40;
  const char* ti[2] = {"Mise à jour disponible", "Copie terminée"}; const char* tx[2] = {"BAURA OS 0.2 est prêt à installer.", "12 fichiers copiés vers Documents."}; const char* ico[2] = {"download", "circle-check"};
  uint32_t c1[2] = {0xFF60A5FA, 0xFF34D399}, c2[2] = {0xFF1D4ED8, 0xFF047857};
  for (int i = 0; i < 1; ++i) { Rect t{x, y, r.w - 36, 66}; u.p.fill_rrect(t, 18, u.T.field); u.p.stroke_rrect(t, 18, 1, u.T.field_line); app_tile(u, {t.x + 12, t.y + 13, 40, 40}, ico[i], c1[i], c2[i]); u.font.draw(u.p, t.x + 64, t.y + 28, ti[i], 14, u.T.fg, Weight::SemiBold); u.font.draw(u.p, t.x + 64, t.y + 48, u.font.ellipsize(tx[i], t.w - 80, 13), 13, u.T.mut); y += 74; }
  y += 6; const char* qi[6] = {"wifi", "bluetooth", "wifi-off", "moon", "battery-charging", "moon"}; const char* ql[6] = {"Wi-Fi", "Bluetooth", "Mode avion", "Ne pas déranger", "Économie", "Thème sombre"}; bool on[6] = {1, 1, 0, 0, 0, 1};
  for (int i = 0; i < 6; ++i) { Rect t{x + (i % 2) * 182.f, y + (i / 2) * 70.f, 174, 62}; if (on[i]) u.p.fill_rrect(t, 18, Paint::linear(145, u.T.b1, u.T.b2)); else { u.p.fill_rrect(t, 18, u.T.field); u.p.stroke_rrect(t, 18, 1, u.T.field_line); }
    u.p.icon(qi[i], {t.x + 14, t.y + 19, 24, 24}, on[i] ? 0xFFFFFFFF : u.T.fg); u.font.draw(u.p, t.x + 48, t.y + 30, ql[i], 13, on[i] ? 0xFFFFFFFF : u.T.fg, Weight::SemiBold); u.font.draw(u.p, t.x + 48, t.y + 47, on[i] ? "Activé" : "Désactivé", 11, on[i] ? white(.8f) : u.T.mut); }
  y += 3 * 70 + 10; u.p.icon("volume-2", {x + 2, y + 2, 20, 20}, u.T.fg); slider(u, {x + 34, y, r.w - 36 - 40, 24}, .62f); y += 38; u.p.icon("sun", {x + 2, y + 2, 20, 20}, u.T.fg); slider(u, {x + 34, y, r.w - 36 - 40, 24}, .80f); y += 40;
  Rect m{x, y, r.w - 36, 82}; u.p.fill_rrect(m, 18, u.T.field); u.p.stroke_rrect(m, 18, 1, u.T.field_line); u.p.fill_rrect({m.x + 12, m.y + 13, 56, 56}, 14, Paint::linear(145, 0xFFF472B6, 0xFFBE185D)); u.p.icon("music", {m.x + 26, m.y + 27, 28, 28}, 0xFFFFFFFF);
  u.font.draw(u.p, m.x + 80, m.y + 32, "Nuit indigo", 14, u.T.fg, Weight::SemiBold); u.font.draw(u.p, m.x + 80, m.y + 50, "Lumen · BMedia", 12, u.T.mut); progress(u, {m.x + 80, m.y + 62, 120, 8}, .4f);
  u.p.icon("skip-back", {m.x + m.w - 102, m.y + 29, 22, 22}, u.T.fg); u.p.fill_rrect({m.x + m.w - 74, m.y + 24, 32, 32}, 16, u.T.fg); u.p.icon("pause", {m.x + m.w - 68, m.y + 30, 20, 20}, u.T.fg == BauraBrand::LIGHT.fg ? 0xFFFFFFFF : 0xFF17133D); u.p.icon("skip-forward", {m.x + m.w - 34, m.y + 29, 22, 22}, u.T.fg);
  return r;
}

// ---- Connexion / verrouillage ----
inline void login(Ui& u, int W, int H, bool error) {
  Painter& p = u.p; const auto& T = u.T; Rect c{(W - 380) / 2.f, H * .5f - 190, 380, 400}; panel(u, c, 32, true);
  u.font.draw(p, W / 2.f - u.font.measure("12:04", 44, Weight::SemiBold) / 2, H * .17f, "12:04", 44, T.fg, Weight::SemiBold);
  float cw = u.font.measure("mercredi 14 octobre", 14); u.font.draw(p, W / 2.f - cw / 2, H * .17f + 26, "mercredi 14 octobre", 14, T.mut);
  Rect av{c.x + c.w / 2 - 40, c.y + 30, 80, 80}; p.fill_rrect(av, 40, Paint::linear(145, T.b1, T.b2)); p.stroke_rrect(av, 40, 2, white(.5f)); text_c(u, av, "A", 32, 0xFFFFFFFF, Weight::SemiBold);
  text_c(u, {c.x, c.y + 120, c.w, 30}, "Ahmed", 20, T.fg, Weight::SemiBold); text_c(u, {c.x, c.y + 148, c.w, 20}, "Compte administrateur", 12, T.mut);
  input(u, {c.x + 30, c.y + 188, c.w - 60, 44}, "••••••••", "", error ? State::Error : State::Focus, "lock", false, 8); u.p.icon("eye", {c.x + c.w - 30 - 34, c.y + 188 + 11, 22, 22}, T.mut);
  button(u, {c.x + 30, c.y + 248, c.w - 60, 42}, "Se connecter", Kind::Primary);
  if (error) { p.icon("circle-alert", {c.x + 30, c.y + 304, 16, 16}, T.err, 2.2f); u.font.draw(p, c.x + 52, c.y + 317, "Mot de passe incorrect — réessayez", 13, T.err); }
  text_c(u, {c.x, c.y + 340, c.w, 24}, "Changer d'utilisateur", 13, T.accent, Weight::SemiBold);
  const char* bi[3] = {"accessibility", "languages", "power"}; for (int i = 0; i < 3; ++i) { Rect b{W - 28.f - 44 * (3 - i), H - 28.f - 40, 40, 40}; panel(u, b, 14); p.icon(bi[i], {b.x + 10, b.y + 10, 20, 20}, T.fg); }
  Rect net{28.f, H - 68.f, 120, 40}; panel(u, net, 14); p.icon("wifi", {net.x + 12, net.y + 10, 20, 20}, T.fg); p.icon("battery-medium", {net.x + 44, net.y + 10, 20, 20}, T.fg); u.font.draw(p, net.x + 72, net.y + 25, "84 %", 12, T.fg, Weight::SemiBold);
}

// ---- Boîte de dialogue UAC (élévation) sur fond assombri ----
inline void uac(Ui& u, int W, int H) {
  u.p.fill_rrect({0, 0, float(W), float(H)}, 0, 0x8C0B0830); Rect r{(W - 460) / 2.f, (H - 330) / 2.f, 460, 330}; panel(u, r, 26, true);
  u.p.fill_rrect({r.x, r.y, r.w, 6}, 0, u.T.b4); Rect ic{r.x + 28, r.y + 32, 56, 56}; u.p.fill_rrect(ic, 18, u.a(u.T.b4, .2f)); u.p.icon("shield", {ic.x + 12, ic.y + 12, 32, 32}, u.T.b4, 2.2f);
  u.font.draw(u.p, r.x + 100, r.y + 56, "Autoriser cette application ?", 17, u.T.fg, Weight::SemiBold); u.font.draw(u.p, r.x + 100, r.y + 78, "Elle demande des droits d'administrateur.", 13, u.T.mut);
  Rect det{r.x + 28, r.y + 108, r.w - 56, 74}; u.p.fill_rrect(det, 16, u.T.field); u.p.stroke_rrect(det, 16, 1, u.T.field_line);
  app_tile(u, {det.x + 14, det.y + 15, 44, 44}, "terminal", 0xFF34D399, 0xFF047857); u.font.draw(u.p, det.x + 70, det.y + 32, "Terminal", 14, u.T.fg, Weight::SemiBold); u.font.draw(u.p, det.x + 70, det.y + 52, "Éditeur vérifié : BAURA Project", 12, u.T.mut);
  input(u, {r.x + 28, r.y + 200, r.w - 56, 42}, "", "Mot de passe de l'administrateur", State::Focus, "key-round");
  button(u, {r.x + 28, r.y + 262, 200, 40}, "Autoriser", Kind::Primary); button(u, {r.x + 240, r.y + 262, 192, 40}, "Refuser", Kind::Secondary);
}

// ---- Bulle système : progression d'opération + graphique de charge ----
inline void system_bubble(Ui& u, Vec2 pos, float prog, float phase) {
  Rect a{pos.x, pos.y, 320, 76}; panel(u, a, 22, true); u.p.fill_rrect({a.x + 14, a.y + 14, 48, 48}, 15, Paint::linear(145, u.T.b1, u.T.b2)); u.p.symbol({a.x + 28, a.y + 22, 20, 32}, 0xFFFFFFFF, white(.85f), white(.7f), u.T.b4);
  u.font.draw(u.p, a.x + 76, a.y + 33, "Copie du système", 14, u.T.fg, Weight::SemiBold); progress(u, {a.x + 76, a.y + 46, a.w - 94, 8}, prog); u.font.draw(u.p, a.x + 76, a.y + 68, std::to_string(int(prog * 100)) + " % · 2 min restantes", 12, u.T.mut);
  Rect g{pos.x, pos.y + 88, 320, 120}; panel(u, g, 22); u.font.draw(u.p, g.x + 18, g.y + 28, "Processeur", 13, u.T.fg, Weight::SemiBold); u.font.draw(u.p, g.x + g.w - 54, g.y + 28, "37 %", 13, u.T.accent, Weight::SemiBold);
  Path line; std::vector<Vec2> pts; for (int i = 0; i <= 40; ++i) { float t = i / 40.f; float v = .38f + .22f * std::sin(t * 9 + phase) * std::sin(t * 3.1f + 1) + .08f * std::sin(t * 31 + phase * 2); pts.push_back({g.x + 18 + t * (g.w - 36), g.y + g.h - 18 - v * 62}); }
  Path area; area.subs.push_back({}); auto& s = area.subs.back(); s.pts.push_back({pts.front().x, g.y + g.h - 18}); for (auto& q : pts) s.pts.push_back(q); s.pts.push_back({pts.back().x, g.y + g.h - 18}); s.closed = true;
  u.p.fill_path(area, {0, 0}, 1, Paint::linear(180, u.a(u.T.b2, .45f), u.a(u.T.b2, 0.f))); line.subs.push_back({}); line.subs.back().pts = pts; u.p.stroke_path(line, {0, 0}, 1, 2.2f, u.T.b2);
}
}  // namespace BauraShell
#endif
