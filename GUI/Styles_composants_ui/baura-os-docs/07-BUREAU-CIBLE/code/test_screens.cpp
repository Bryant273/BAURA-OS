// Test 07 : rend tous les écrans cibles de la coque et les 12 modèles d'applications (clair + sombre pour le bureau).
#include "baura_apps.h"
#include "bmp_out.h"
#include <cstdio>
using namespace BauraShell; using namespace BauraApps;
struct Canvas { int W, H; std::vector<uint32_t> px; BGEFramebuffer fb; BGERenderer r; Painter p; Canvas(int w, int h) : W(w), H(h), px(size_t(w) * h), fb{px.data(), uint32_t(w), uint32_t(h), uint32_t(w), px.size()}, r(fb), p(r) {} };
static TaskbarState tb(int active, int open) { TaskbarState s; const char* a[] = {"files", "houb", "terminal", "media", "store", "settings"}; for (int i = 0; i < 6; ++i) s.pinned[i] = a[i]; s.n = 6; s.active = active; s.open_mask = open; return s; }
static void save(Canvas& c, const char* name) { std::string f = std::string(name) + ".ppm"; write_ppm(f.c_str(), c.px, c.W, c.H); }
int main() {
  auto fb = read_file("../../common/inter.bgf"); Font font; font.load(fb.data(), fb.size()); auto mb = read_file("../../common/mono.bgf"); Font mono; mono.load(mb.data(), mb.size());
  // Coque : disposition par défaut (barre en deux parties + widgets en haut à droite) ; les autres dispositions sont des OPTIONS.
  auto chrome = [&](Ui& u, Layout l, int active, int open, bool start = false, bool notif_open = false, int notifs = 2) -> DockRects {
    TaskbarState ts = tb(active, open); StatusState st; MediaState md; DockRects dr;
    if (l == Layout::SplitDock) { top_widgets(u, u.p.width(), md, "41°C", notifs, notif_open); dr = dock_split(u, u.p.width(), u.p.height(), ts, st, start); }
    else if (l == Layout::FullWidth) { top_widgets(u, u.p.width(), md, "41°C", notifs, notif_open); dr = dock_full(u, u.p.width(), u.p.height(), ts, st, start); }
    else { tray(u, u.p.width(), "41°C", notifs, notif_open); Rect t = taskbar(u, u.p.width(), u.p.height(), ts, start); dr.left = dr.right = t; }
    return dr; };
  for (int d = 0; d < 2; ++d) { const auto& T = d ? BauraBrand::DARK : BauraBrand::LIGHT; const char* sfx = d ? "sombre" : "clair"; char n[64];
    { Canvas c(1280, 720); Ui u{c.p, font, T}; wallpaper(u, 1280, 720, d); Rect w{150, 80, 760, 470}; Rect body = window_frame(u, w, "Fichiers — Documents", "folder", 0xFF6D66F2, 0xFF4F46E5, true);
      Ctx cx{u, mono}; c.p.set_clip(int(body.x), int(body.y), int(body.w), int(body.h)); files(cx, body); c.p.reset_clip(); window_frame(u, {640, 250, 520, 330}, "Terminal", "terminal", 0xFF34D399, 0xFF047857, false, true);
      c.p.set_clip(640, 296, 520, 284); terminal(cx, {640, 296, 520, 284}); c.p.reset_clip(); chrome(u, Layout::SplitDock, 0, 0b000101); std::snprintf(n, 64, "bureau_defaut_%s", sfx); save(c, n); }
    { Canvas c(1280, 720); Ui u{c.p, font, T}; wallpaper(u, 1280, 720, d); DockRects dr = chrome(u, Layout::SplitDock, -1, 0, true); start_menu(u, dr.left, true); std::snprintf(n, 64, "demarrer_%s", sfx); save(c, n); }
    if (d == 0) for (int l = 1; l < 3; ++l) { Canvas c(1280, 720); Ui u{c.p, font, T}; wallpaper(u, 1280, 720, 0); Rect w{150, 80, 760, 470}; Rect body = window_frame(u, w, "Fichiers — Documents", "folder", 0xFF6D66F2, 0xFF4F46E5, true); Ctx cx{u, mono}; c.p.set_clip(int(body.x), int(body.y), int(body.w), int(body.h)); files(cx, body); c.p.reset_clip();
      chrome(u, Layout(l), 0, 0b000101); save(c, l == 1 ? "option_barre-flottante_clair" : "option_pleine-largeur_clair"); }
    if (d == 1) {
      { Canvas c(1280, 720); Ui u{c.p, font, T}; wallpaper(u, 1280, 720, 1); DockRects dr = chrome(u, Layout::SplitDock, -1, 0); search_panel(u, dr.left, "Param", true); save(c, "recherche_sombre"); }
      { Canvas c(1280, 720); Ui u{c.p, font, T}; wallpaper(u, 1280, 720, 1); chrome(u, Layout::SplitDock, -1, 0, false, true); action_center(u, 1280, 720); save(c, "centre-actions_sombre"); }
      { Canvas c(1280, 720); Ui u{c.p, font, T}; wallpaper(u, 1280, 720, 1); login(u, 1280, 720, true); save(c, "connexion_sombre"); }
      { Canvas c(1280, 720); Ui u{c.p, font, T}; wallpaper(u, 1280, 720, 1); Rect w{200, 120, 640, 400}; window_frame(u, w, "Fichiers", "folder", 0xFF6D66F2, 0xFF4F46E5, false); chrome(u, Layout::SplitDock, -1, 1, false, false, 0); uac(u, 1280, 720); save(c, "uac_sombre"); }
      { Canvas c(1280, 720); Ui u{c.p, font, T}; wallpaper(u, 1280, 720, 1); chrome(u, Layout::SplitDock, -1, 0, false, false, 1); system_bubble(u, {28, 76}, .62f, 1.f); menu(u, {700, 300}, 240, {{"Ouvrir", "folder-open", "Entrée"}, {"Copier", "copy", "Ctrl+C"}, {"Renommer", "pencil", "F2"}, {"", nullptr, "", true}, {"Propriétés", "info", "Alt+Entrée"}, {"Supprimer", "trash-2", "Suppr", false, false, false, false, true}}, 1); save(c, "bulle-et-menu_sombre"); } }
  }
  // Installateur : 4 étapes
  const char* steps[6] = {"Bienvenue", "Analyse des disques", "Partitionnement", "Formatage", "Copie du système", "Terminé"};
  for (int s = 0; s < 4; ++s) { int idx[4] = {0, 2, 4, 5}; int cur = idx[s]; Canvas c(1280, 720); Ui u{c.p, font, BauraBrand::LIGHT}; wallpaper(u, 1280, 720, false); Rect cd{150, 70, 980, 580}; panel(u, cd, 32, true);
    u.p.symbol({cd.x + 36, cd.y + 34, 30, 48}, u.T.b1, u.T.b2, u.T.b3, u.T.b4); u.font.draw(u.p, cd.x + 80, cd.y + 66, "BAURA OS", 18, u.T.fg, Weight::SemiBold);
    for (int i = 0; i < 6; ++i) { Rect r{cd.x + 24, cd.y + 120 + i * 48.f, 250, 40}; bool on = i == cur, done = i < cur; if (on) u.p.fill_rrect(r, 14, u.a(u.T.b1, .16f)); u.p.fill_rrect({r.x + 10, r.y + 8, 24, 24}, 12, done ? u.T.ok : on ? u.T.b1 : u.T.hair); if (done) u.p.icon("check", {r.x + 14, r.y + 12, 16, 16}, 0xFFFFFFFF, 2.8f); else text_c(u, {r.x + 10, r.y + 8, 24, 24}, std::to_string(i + 1), 12, on ? 0xFFFFFFFF : u.T.mut, Weight::SemiBold); text_l(u, r.x + 46, r, steps[i], 14, on ? u.T.accent : done ? u.T.fg : u.T.mut, on ? Weight::SemiBold : Weight::Regular); }
    u.p.fill_rrect({cd.x + 300, cd.y + 30, 1, cd.h - 60}, 0, u.T.hair); float x = cd.x + 340, w = cd.w - 380; u.font.draw(u.p, x, cd.y + 74, steps[cur], 28, u.T.fg, Weight::SemiBold);
    if (s == 0) { u.font.draw(u.p, x, cd.y + 108, "Un système simple, calme et sûr. L'installation prend environ 10 minutes.", 14, u.T.mut); H(u, x, cd.y + 164, "LANGUE"); Rect l{x, cd.y + 176, 300, 44}; input(u, l, "Français (France)", "", State::Normal, "languages"); u.p.icon("chevron-down", {l.x + 268, l.y + 12, 20, 20}, u.T.mut); H(u, x, cd.y + 250, "DISPOSITION DU CLAVIER"); l = {x, cd.y + 262, 300, 44}; input(u, l, "AZERTY — français", "", State::Normal, "keyboard"); u.p.icon("chevron-down", {l.x + 268, l.y + 12, 20, 20}, u.T.mut); card(u, {x, cd.y + 340, w, 70}); text_l(u, x + 20, {x, cd.y + 340, w, 70}, "Astuce : maintenez Maj+F10 pour ouvrir le mode accessibilité.", 13, u.T.mut); }
    if (s == 1) { u.font.draw(u.p, x, cd.y + 108, "Choisissez le disque où installer BAURA OS. Les données de ce disque seront effacées.", 14, u.T.mut);
      const char* dn[2] = {"Disque 1 — NVMe 512 Go", "Disque 2 — USB 32 Go"}; for (int i = 0; i < 2; ++i) { Rect r{x, cd.y + 150 + i * 120.f, w, 104}; u.p.fill_rrect(r, 20, i == 0 ? u.a(u.T.b1, .12f) : u.T.field); u.p.stroke_rrect(r, 20, i == 0 ? 2.f : 1.f, i == 0 ? u.T.b1 : u.T.field_line); u.p.icon(i ? "usb" : "hard-drive", {r.x + 20, r.y + 20, 28, 28}, u.T.fg); u.font.draw(u.p, r.x + 64, r.y + 38, dn[i], 15, u.T.fg, Weight::SemiBold);
        float bw = w - 40; u.p.fill_rrect({r.x + 20, r.y + 62, bw, 14}, 7, u.T.hair); if (i == 0) { u.p.fill_rrect({r.x + 20, r.y + 62, bw * .06f, 14}, 7, u.T.b4); u.p.fill_rrect({r.x + 20 + bw * .07f, r.y + 62, bw * .93f, 14}, 7, Paint::linear(90, u.T.b1, u.T.b2)); } u.font.draw(u.p, r.x + 20, r.y + 96, i ? "32 Go — 1 partition existante" : "EFI 512 Mo + BAURA OS 475 Go (nouvelle disposition)", 12, u.T.mut); if (i == 0) u.p.icon("circle-check", {r.x + r.w - 44, r.y + 18, 24, 24}, u.T.b1, 2.2f); }
      Rect wr{x, cd.y + 400, w, 52}; u.p.fill_rrect(wr, 16, u.a(u.T.b4, .16f)); u.p.icon("triangle-alert", {wr.x + 16, wr.y + 14, 24, 24}, u.T.b4, 2.2f); text_l(u, wr.x + 52, wr, "Toutes les données du disque 1 seront effacées.", 13, u.T.fg, Weight::SemiBold); }
    if (s == 2) { u.font.draw(u.p, x, cd.y + 108, "Ne fermez pas la fenêtre et ne débranchez pas l'appareil.", 14, u.T.mut); progress(u, {x, cd.y + 170, w, 8}, .58f); u.font.draw(u.p, x, cd.y + 206, "Copie des fichiers système… 58 %", 14, u.T.fg, Weight::SemiBold); u.font.draw(u.p, x + w - 150, cd.y + 206, "environ 4 min", 13, u.T.mut);
      const char* lg[5] = {"✓ Partitions créées", "✓ Système de fichiers formaté", "✓ Noyau seL4 copié", "→ Services BAURA Core…", "· Thème et polices"}; for (int i = 0; i < 5; ++i) u.font.draw(u.p, x, cd.y + 256 + i * 28, lg[i], 14, i < 3 ? u.T.ok : i == 3 ? u.T.fg : u.T.mut); }
    if (s == 3) { Rect ok{x + w / 2 - 50, cd.y + 150, 100, 100}; u.p.fill_rrect(ok, 50, u.a(u.T.ok, .16f)); u.p.icon("check", {ok.x + 26, ok.y + 26, 48, 48}, u.T.ok, 3.f); text_c(u, {x, cd.y + 270, w, 30}, "BAURA OS est installé", 20, u.T.fg, Weight::SemiBold); text_c(u, {x, cd.y + 300, w, 24}, "Retirez le support d'installation puis redémarrez.", 14, u.T.mut); }
    if (s == 3) button(u, {x + w / 2 - 90, cd.y + 350, 180, 42}, "Redémarrer", Kind::Primary, State::Normal, "rotate-cw"); else { button(u, {x + w - 140, cd.y + cd.h - 78, 120, 42}, "Continuer", Kind::Primary); if (s) button(u, {x + w - 270, cd.y + cd.h - 78, 112, 42}, "Retour", Kind::Secondary); }
    char n[64]; std::snprintf(n, 64, "installateur_%d", s + 1); save(c, n); }
  // Applications
  struct AF { const char* id; const char* title; void (*f)(Ctx&, Rect); } afs[] = {{"files", "Fichiers", files}, {"settings", "Paramètres", settings}, {"terminal", "Terminal", terminal}, {"tasks", "Gestionnaire des tâches", tasks}, {"store", "Boutique", store}, {"houb", "HOUB", houb}, {"media", "BMedia", media}, {"sound", "Son", sound}, {"security", "Centre de sécurité", security}, {"update", "Mise à jour du système", update}, {"power", "Centre d'énergie", power}, {"network", "Réseau", network}};
  for (int i = 0; i < 12; ++i) { bool dk = i % 2; const auto& T = dk ? BauraBrand::DARK : BauraBrand::LIGHT; Canvas c(1280, 800); Ui u{c.p, font, T}; wallpaper(u, 1280, 800, dk); const AppDef& a = app(afs[i].id);
    Rect body = window_frame(u, {80, 60, 1120, 640}, afs[i].title, a.icon, a.c1, a.c2, true, i == 2 || i == 3 || i == 9); Ctx cx{u, mono}; c.p.set_clip(int(body.x), int(body.y), int(body.w), int(body.h)); afs[i].f(cx, body); c.p.reset_clip(); chrome(u, Layout::SplitDock, i < 6 ? i : -1, 1 << (i % 6), false, false, 1); char n[64]; std::snprintf(n, 64, "app_%02d_%s", i + 1, afs[i].id); save(c, n); }
  // Paramètres > Bureau : la disposition par défaut (barre coupée en deux) et ses options de personnalisation
  for (int dk = 0; dk < 2; ++dk) { const auto& T = dk ? BauraBrand::DARK : BauraBrand::LIGHT; Canvas c(1280, 800); Ui u{c.p, font, T}; wallpaper(u, 1280, 800, dk);
    Rect body = window_frame(u, {80, 60, 1120, 640}, "Paramètres", "settings", 0xFF9AA0C8, 0xFF6B7199, true); Ctx cx{u, mono}; c.p.set_clip(int(body.x), int(body.y), int(body.w), int(body.h)); desktop_settings(cx, body, 0, false, dk != 0); c.p.reset_clip(); save(c, dk ? "reglage_bureau_sombre" : "reglage_bureau_clair"); }
  std::printf("07 : rendus écrits\n"); return 0;
}
