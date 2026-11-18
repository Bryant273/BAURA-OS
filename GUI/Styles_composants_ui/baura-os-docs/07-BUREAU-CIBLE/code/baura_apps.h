// BAURA OS — baura_apps.h : modèles (gabarits) des 12 applications système, dessinés dans le cadre de fenêtre BAURA.
// Chaque fonction reçoit la zone de contenu (sous la barre de 46 px) et dessine un état représentatif.
#ifndef BAURA_APPS_H
#define BAURA_APPS_H
#include "baura_shell.h"
namespace BauraApps {
using namespace BauraShell;
struct Ctx { Ui& u; const Font& mono; };
inline void hair(Ui& u, float x, float y, float w) { u.p.fill_rrect({x, y, w, 1}, 0, u.T.hair); }
inline void H(Ui& u, float x, float y, const std::string& s) { u.font.draw(u.p, x, y, s, 12, u.T.mut, Weight::SemiBold); }
inline void card(Ui& u, Rect r, float rad = 18) { u.p.fill_rrect(r, rad, u.T.field); u.p.stroke_rrect(r, rad, 1, u.T.field_line); }
inline void sidebar(Ui& u, Rect c, float w, const std::vector<std::pair<const char*, const char*>>& items, int sel) {
  u.p.fill_rrect({c.x, c.y, w, c.h}, 0, u.a(u.T.b3, .10f)); u.p.fill_rrect({c.x + w - 1, c.y, 1, c.h}, 0, u.T.hair);
  for (size_t i = 0; i < items.size(); ++i) { Rect r{c.x + 10, c.y + 12 + i * 40.f, w - 20, 36}; if (int(i) == sel) { u.p.fill_rrect(r, 12, u.a(u.T.b1, .16f)); u.p.fill_rrect({r.x, r.y + 9, 3, 18}, 1.5f, u.T.b1); }
    u.p.icon(items[i].first, {r.x + 12, r.y + 8, 20, 20}, int(i) == sel ? u.T.accent : u.T.fg); text_l(u, r.x + 42, r, items[i].second, 14, int(i) == sel ? u.T.accent : u.T.fg, int(i) == sel ? Weight::SemiBold : Weight::Regular); }
}
inline void ring(Ui& u, Vec2 c, float rad, float w, float v, uint32_t col, const std::string& big, const std::string& small) {
  Path bg, fg; bg.subs.push_back({}); fg.subs.push_back({}); for (int i = 0; i <= 96; ++i) { float a = -1.5708f + i / 96.f * 6.2832f; bg.subs[0].pts.push_back({c.x + rad * std::cos(a), c.y + rad * std::sin(a)}); }
  int n = int(96 * v); for (int i = 0; i <= n; ++i) { float a = -1.5708f + i / 96.f * 6.2832f; fg.subs[0].pts.push_back({c.x + rad * std::cos(a), c.y + rad * std::sin(a)}); }
  u.p.stroke_path(bg, {0, 0}, 1, w, u.T.hair); u.p.stroke_path(fg, {0, 0}, 1, w, col);
  text_c(u, {c.x - rad, c.y - 20, 2 * rad, 28}, big, 28, u.T.fg, Weight::SemiBold); text_c(u, {c.x - rad, c.y + 10, 2 * rad, 18}, small, 12, u.T.mut);
}
inline void graph(Ui& u, Rect r, uint32_t col, float seed) {
  card(u, r); std::vector<Vec2> pts; for (int i = 0; i <= 48; ++i) { float t = i / 48.f, v = .45f + .25f * std::sin(t * 7 + seed) * std::sin(t * 2.3f + seed) + .06f * std::sin(t * 29 + seed); pts.push_back({r.x + 12 + t * (r.w - 24), r.y + r.h - 12 - v * (r.h - 30)}); }
  Path a; a.subs.push_back({}); a.subs[0].pts.push_back({pts.front().x, r.y + r.h - 12}); for (auto& q : pts) a.subs[0].pts.push_back(q); a.subs[0].pts.push_back({pts.back().x, r.y + r.h - 12}); a.subs[0].closed = true;
  u.p.fill_path(a, {0, 0}, 1, Paint::linear(180, u.a(col, .38f), u.a(col, 0))); Path l; l.subs.push_back({}); l.subs[0].pts = pts; u.p.stroke_path(l, {0, 0}, 1, 2.2f, col);
}
inline void file_tile(Ui& u, Rect r, const char* icon, uint32_t c1, uint32_t c2, const char* name, bool sel) {
  if (sel) { u.p.fill_rrect(r, 16, u.a(u.T.b1, .16f)); u.p.stroke_rrect(r, 16, 1.5f, u.T.b1); }
  app_tile(u, {r.x + r.w / 2 - 28, r.y + 10, 56, 56}, icon, c1, c2); text_c(u, {r.x, r.y + 72, r.w, 20}, u.font.ellipsize(name, r.w - 8, 13), 13, u.T.fg);
}

inline void files(Ctx& c, Rect b) { Ui& u = c.u;
  sidebar(u, b, 200, {{"house", "Accueil"}, {"monitor", "Bureau"}, {"file-text", "Documents"}, {"image", "Images"}, {"music", "Musique"}, {"download", "Téléchargements"}, {"trash-2", "Corbeille"}}, 2);
  float x = b.x + 216, w = b.w - 232; icon_button(u, {x, b.y + 10, 34, 32}, "arrow-left"); icon_button(u, {x + 38, b.y + 10, 34, 32}, "arrow-right", State::Disabled); icon_button(u, {x + 76, b.y + 10, 34, 32}, "arrow-up");
  Rect path{x + 120, b.y + 10, w - 120 - 230, 32}; u.p.fill_rrect(path, 12, u.T.field); u.p.stroke_rrect(path, 12, 1, u.T.field_line); u.p.icon("folder", {path.x + 10, path.y + 7, 18, 18}, u.T.mut); text_l(u, path.x + 36, path, "Accueil", 13, u.T.mut); u.p.icon("chevron-right", {path.x + 84, path.y + 8, 16, 16}, u.T.mut); text_l(u, path.x + 104, path, "Documents", 13, u.T.fg, Weight::SemiBold);
  input(u, {x + w - 220, b.y + 6, 160, 40}, "", "Rechercher", State::Normal, "search"); icon_button(u, {x + w - 52, b.y + 10, 34, 32}, "layout-grid", State::Normal, true);
  struct F { const char* i; uint32_t a, b; const char* n; } fs[] = {{"folder", 0xFF6D66F2, 0xFF4F46E5, "Projets"}, {"folder", 0xFF6D66F2, 0xFF4F46E5, "Factures"}, {"file-text", 0xFF60A5FA, 0xFF1D4ED8, "Rapport T3.docx"}, {"table", 0xFF34D399, 0xFF047857, "Budget 2026.xlsx"}, {"image", 0xFFF472B6, 0xFFBE185D, "Vacances.png"}, {"presentation", 0xFFFB923C, 0xFFEA580C, "Pitch.pptx"}, {"music", 0xFFA78BFA, 0xFF6D28D9, "Lumen.flac"}, {"archive", 0xFF9AA0C8, 0xFF6B7199, "Sauvegarde.zip"}, {"file-code", 0xFF34D399, 0xFF0F766E, "notes.md"}, {"film", 0xFFF87171, 0xFFB91C1C, "Demo.mp4"}};
  H(u, x + 4, b.y + 78, "10 ÉLÉMENTS"); for (int i = 0; i < 10; ++i) file_tile(u, {x + (i % 5) * 150.f, b.y + 92 + (i / 5) * 118.f, 140, 106}, fs[i].i, fs[i].a, fs[i].b, fs[i].n, i == 2);
  hair(u, b.x + 201, b.y + b.h - 34, b.w - 201); u.font.draw(u.p, x + 4, b.y + b.h - 12, "10 éléments · 1 sélectionné (284 Ko)", 12, u.T.mut);
}
inline void settings(Ctx& c, Rect b) { Ui& u = c.u;
  sidebar(u, b, 230, {{"palette", "Apparence"}, {"monitor", "Affichage"}, {"wifi", "Réseau"}, {"volume-2", "Son"}, {"lock", "Confidentialité"}, {"user", "Comptes"}, {"languages", "Langue et région"}, {"accessibility", "Accessibilité"}}, 0);
  float x = b.x + 262, w = b.w - 294; u.font.draw(u.p, x, b.y + 44, "Apparence", 28, u.T.fg, Weight::SemiBold);
  H(u, x, b.y + 84, "THÈME"); for (int i = 0; i < 3; ++i) { Rect t{x + i * 196.f, b.y + 96, 184, 116}; bool on = i == 0; card(u, t); if (on) u.p.stroke_rrect(t, 18, 2.5f, u.T.b1);
    Rect pv{t.x + 12, t.y + 12, t.w - 24, 62}; u.p.fill_rrect(pv, 12, i == 1 ? Paint::linear(150, 0xFF1A1450, 0xFF3B1F8F) : i == 0 ? Paint::linear(150, 0xFFF1E4FF, 0xFFA99BFF) : Paint::linear(150, 0xFFF1E4FF, 0xFF3B1F8F)); u.p.fill_rrect({pv.x + 14, pv.y + 22, pv.w - 28, 28}, 8, i == 1 ? 0x55FFFFFF & 0x33FFFFFF : 0x99FFFFFF);
    const char* n[3] = {"Clair", "Sombre", "Automatique"}; u.font.draw(u.p, t.x + 14, t.y + 98, n[i], 13, u.T.fg, Weight::SemiBold); if (on) u.p.icon("circle-check", {t.x + t.w - 32, t.y + 84, 18, 18}, u.T.b1, 2.2f); }
  H(u, x, b.y + 246, "COULEUR D'ACCENT"); uint32_t sw[5] = {0xFF4F46E5, 0xFF8B5CF6, 0xFF0EA5E9, 0xFF10B981, 0xFFAB8E18}; for (int i = 0; i < 5; ++i) { Rect s{x + i * 46.f, b.y + 258, 36, 36}; u.p.fill_rrect(s, 18, sw[i]); if (i == 0) { u.p.stroke_rrect({s.x - 4, s.y - 4, 44, 44}, 22, 2, u.T.b1); u.p.icon("check", {s.x + 9, s.y + 9, 18, 18}, 0xFFFFFFFF, 2.6f); } }
  H(u, x, b.y + 326, "EFFETS"); Rect g{x, b.y + 338, w, 168}; card(u, g, 20);
  const char* n[3] = {"Transparence et flou du verre", "Animations", "Réduire les mouvements"}; const char* d[3] = {"Fond translucide sur la barre et les menus", "Ressorts à l'ouverture et à la fermeture", "Désactive les effets de déplacement"}; bool on[3] = {1, 1, 0};
  for (int i = 0; i < 3; ++i) { float y = g.y + i * 56; u.font.draw(u.p, g.x + 20, y + 24, n[i], 14, u.T.fg, Weight::SemiBold); u.font.draw(u.p, g.x + 20, y + 42, d[i], 12, u.T.mut); toggle(u, {g.x + g.w - 64, y + 15, 44, 26}, on[i]); if (i < 2) hair(u, g.x + 16, y + 55, g.w - 32); }
}
inline void terminal(Ctx& c, Rect b) { Ui& u = c.u; const Font& m = c.mono; Painter& p = u.p;
  p.fill_rrect(b, 0, 0xF00F0B2E); p.set_clip(int(b.x), int(b.y), int(b.w), int(b.h));
  struct L { const char* s; uint32_t col; } ls[] = {{"ahmed@baura:~$ baura version", 0xFFF4F2FF}, {"BAURA OS 0.2.0 · noyau seL4 · x86_64", 0xFFB4B0DA}, {"ahmed@baura:~$ ls Documents", 0xFFF4F2FF}, {"Factures   Projets   Rapport T3.docx   notes.md", 0xFFCFC3FF}, {"ahmed@baura:~$ sudo baura update --check", 0xFFF4F2FF}, {"[sudo] mot de passe : ********", 0xFFB4B0DA}, {"✓ Une mise à jour est disponible : 0.2.1", 0xFF5FD49B}, {"→ Lancez « baura update --apply » pour l'installer.", 0xFFD0AE2A}, {"ahmed@baura:~$ ", 0xFFF4F2FF}};
  for (int i = 0; i < 9; ++i) { float y = b.y + 30 + i * 22.f; m.draw(p, b.x + 20, y, ls[i].s, 14, ls[i].col); }
  p.fill_rrect({b.x + 20 + m.measure(ls[8].s, 14) + 1, b.y + 30 + 8 * 22.f - 12, 8, 16}, 2, 0xFF9D77FF); p.reset_clip();
  Rect tb{b.x, b.y, b.w, 0}; (void)tb;
}
inline void tasks(Ctx& c, Rect b) { Ui& u = c.u; Rect tr{b.x + 16, b.y + 10, 420, 40}; u.p.fill_rrect(tr, 14, u.T.win_bar); tabs(u, tr, {{"Processus", "activity", true}, {"Performances", "cpu", false}, {"Démarrage", "zap", false}});
  graph(u, {b.x + 16, b.y + 62, (b.w - 56) / 3, 110}, u.T.b1, 1); graph(u, {b.x + 28 + (b.w - 56) / 3, b.y + 62, (b.w - 56) / 3, 110}, u.T.b2, 3); graph(u, {b.x + 40 + 2 * (b.w - 56) / 3, b.y + 62, (b.w - 56) / 3, 110}, 0xFF34D399, 5);
  const char* gl[3] = {"Processeur  37 %", "Mémoire  4,1 / 16 Go", "Disque  12 Mo/s"}; for (int i = 0; i < 3; ++i) u.font.draw(u.p, b.x + 28 + i * ((b.w - 56) / 3 + 12), b.y + 84, gl[i], 12, u.T.fg, Weight::SemiBold);
  float y = b.y + 190; H(u, b.x + 52, y + 10, "NOM"); H(u, b.x + b.w - 360, y + 10, "PROCESSEUR"); H(u, b.x + b.w - 240, y + 10, "MÉMOIRE"); H(u, b.x + b.w - 110, y + 10, "ÉTAT"); y += 20;
  struct P { const char* n; const char* i; int cpu; const char* mem; const char* st; } ps[] = {{"BAURA Core", "cpu", 8, "212 Mo", "Actif"}, {"Serveur d'affichage", "monitor", 14, "184 Mo", "Actif"}, {"HOUB", "globe", 9, "640 Mo", "Actif"}, {"Fichiers", "folder", 1, "96 Mo", "Actif"}, {"BMedia", "music", 3, "142 Mo", "Suspendu"}, {"Terminal", "terminal", 0, "38 Mo", "Actif"}, {"Antivirus BAURA", "shield-check", 2, "120 Mo", "Actif"}};
  for (int i = 0; i < 7; ++i) { Rect r{b.x + 12, y + i * 44.f, b.w - 24, 40}; if (i == 2) u.p.fill_rrect(r, 12, u.a(u.T.b1, .14f)); else if (i % 2) u.p.fill_rrect(r, 12, u.a(u.T.b3, .08f));
    u.p.icon(ps[i].i, {r.x + 14, r.y + 10, 20, 20}, u.T.fg); text_l(u, r.x + 44, r, ps[i].n, 14, u.T.fg, Weight::SemiBold);
    progress(u, {r.x + r.w - 354, r.y + 16, 70, 8}, std::max(.04f, ps[i].cpu / 30.f)); text_l(u, r.x + r.w - 274, r, std::to_string(ps[i].cpu) + " %", 13, u.T.fg); text_l(u, r.x + r.w - 228, r, ps[i].mem, 13, u.T.fg); text_l(u, r.x + r.w - 104, r, ps[i].st, 13, ps[i].st[0] == 'S' ? u.T.mut : u.T.ok); }
  button(u, {b.x + b.w - 176, b.y + b.h - 54, 160, 38}, "Fin de tâche", Kind::Danger);
}
inline void store(Ctx& c, Rect b) { Ui& u = c.u; Rect hero{b.x + 20, b.y + 14, b.w - 40, 168}; u.p.fill_rrect(hero, 24, Paint::linear(120, u.T.b1, u.T.b2)); u.p.symbol({hero.x + hero.w - 150, hero.y - 20, 110, 220}, white(.2f), white(.2f), white(.2f), white(.35f));
  u.font.draw(u.p, hero.x + 32, hero.y + 52, "À LA UNE", 12, white(.8f), Weight::SemiBold); u.font.draw(u.p, hero.x + 32, hero.y + 92, "Atelier Lumen", 28, 0xFFFFFFFF, Weight::SemiBold); u.font.draw(u.p, hero.x + 32, hero.y + 118, "Retouche photo légère, pensée pour BAURA OS.", 14, white(.9f)); Rect gb{hero.x + 32, hero.y + 128, 120, 34}; u.p.fill_rrect(gb, 12, 0xFFFFFFFF); text_c(u, gb, "Obtenir", 14, u.T.b1 == BauraBrand::LIGHT.b1 ? 0xFF4F46E5 : 0xFF6D66F2, Weight::SemiBold);
  float x = b.x + 20; const char* cs[6] = {"Tout", "Productivité", "Créativité", "Jeux", "Développement", "Utilitaires"}; for (int i = 0; i < 6; ++i) { float w = u.font.measure(cs[i], 13, Weight::SemiBold) + 28; chip(u, {x, b.y + 198, w, 30}, cs[i], i == 0); x += w + 8; }
  struct A { const char* n; const char* d; const char* i; uint32_t a, b; const char* btn; } as[] = {{"Notes", "Prise de notes rapide", "pen-line", 0xFFFBBF24, 0xFFD97706, "Obtenir"}, {"Calcul", "Tableur léger", "table", 0xFF34D399, 0xFF047857, "Obtenir"}, {"Éditeur", "Code & texte", "file-code", 0xFF60A5FA, 0xFF1D4ED8, "Ouvrir"}, {"Dessin", "Vectoriel", "pen-tool", 0xFFF472B6, 0xFFBE185D, "Obtenir"}, {"Échecs", "Contre l'ordinateur", "puzzle", 0xFFA78BFA, 0xFF6D28D9, "Obtenir"}, {"Météo", "Prévisions locales", "cloud-sun", 0xFF38BDF8, 0xFF0369A1, "Installé"}};
  for (int i = 0; i < 6; ++i) { Rect r{b.x + 20 + (i % 3) * ((b.w - 40) / 3.f), b.y + 244 + (i / 3) * 112.f, (b.w - 40) / 3.f - 12, 100}; card(u, r, 20); app_tile(u, {r.x + 14, r.y + 14, 56, 56}, as[i].i, as[i].a, as[i].b); u.font.draw(u.p, r.x + 82, r.y + 36, as[i].n, 15, u.T.fg, Weight::SemiBold); u.font.draw(u.p, r.x + 82, r.y + 56, as[i].d, 12, u.T.mut);
    Rect bt{r.x + r.w - 92, r.y + r.h - 40, 78, 28}; bool inst = as[i].btn[0] == 'I'; u.p.fill_rrect(bt, 14, inst ? u.T.field : u.a(u.T.b1, .16f)); text_c(u, bt, as[i].btn, 12, inst ? u.T.mut : u.T.accent, Weight::SemiBold); u.p.icon("star", {r.x + 82, r.y + 74, 14, 14}, u.T.b4, 2.4f); u.font.draw(u.p, r.x + 100, r.y + 86, "4,6", 11, u.T.b4, Weight::SemiBold); }
}
inline void houb(Ctx& c, Rect b) { Ui& u = c.u; Rect tb{b.x, b.y, b.w, 40}; u.p.fill_rrect(tb, 0, u.T.win_bar); float x = b.x + 12; const char* tt[3] = {"Accueil HOUB", "Docs BAURA", "Cartes"}; for (int i = 0; i < 3; ++i) { Rect t{x, b.y + 6, 190, 34}; if (i == 0) { u.p.fill_rrect(t, 12, u.a(u.T.win, 1.f)); u.p.fill_rrect({t.x, t.y + 20, t.w, 14}, 0, u.a(u.T.win, 1.f)); } u.p.icon(i == 0 ? "globe" : i == 1 ? "bookmark" : "map", {t.x + 12, t.y + 8, 18, 18}, i == 0 ? u.T.accent : u.T.mut); text_l(u, t.x + 38, t, tt[i], 13, i == 0 ? u.T.fg : u.T.mut, i == 0 ? Weight::SemiBold : Weight::Regular); x += 196; } icon_button(u, {x + 4, b.y + 4, 34, 32}, "plus");
  float y = b.y + 48; icon_button(u, {b.x + 12, y, 34, 32}, "arrow-left"); icon_button(u, {b.x + 50, y, 34, 32}, "arrow-right", State::Disabled); icon_button(u, {b.x + 88, y, 34, 32}, "rotate-cw"); Rect ad{b.x + 134, y, b.w - 134 - 56, 32}; u.p.fill_rrect(ad, 16, u.T.field); u.p.stroke_rrect(ad, 16, 1, u.T.field_line); u.p.icon("lock", {ad.x + 14, ad.y + 7, 18, 18}, u.T.ok, 2.2f); text_l(u, ad.x + 42, ad, "houb://accueil", 13, u.T.fg); icon_button(u, {b.x + b.w - 46, y, 34, 32}, "menu");
  Rect page{b.x, y + 44, b.w, b.h - 92}; u.p.fill_rrect(page, 0, Paint::linear(160, u.a(u.T.b3, .22f), u.a(u.T.b2, .10f))); u.p.symbol({page.x + page.w / 2 - 28, page.y + 40, 56, 100}, u.T.b1, u.T.b2, u.T.b3, u.T.b4);
  text_c(u, {page.x, page.y + 150, page.w, 36}, "HOUB", 28, u.T.fg, Weight::SemiBold); input(u, {page.x + page.w / 2 - 260, page.y + 200, 520, 48}, "", "Rechercher ou saisir une adresse", State::Focus, "search");
  for (int i = 0; i < 6; ++i) { const AppDef& a = APPS[i]; float cx = page.x + page.w / 2 - 3 * 92 + i * 92.f; app_tile(u, {cx + 14, page.y + 290, 56, 56}, a.icon, a.c1, a.c2); text_c(u, {cx, page.y + 352, 84, 20}, a.label, 12, u.T.fg); }
}
inline void media(Ctx& c, Rect b) { Ui& u = c.u; sidebar(u, {b.x, b.y, b.w, b.h - 84}, 190, {{"house", "Accueil"}, {"disc-3", "Albums"}, {"mic", "Artistes"}, {"list-music", "Playlists"}, {"heart", "Favoris"}}, 0);
  float x = b.x + 214; u.font.draw(u.p, x, b.y + 44, "Pour vous", 28, u.T.fg, Weight::SemiBold); uint32_t g[5][2] = {{0xFFF472B6, 0xFFBE185D}, {0xFF6D66F2, 0xFF4F46E5}, {0xFFFBBF24, 0xFFD97706}, {0xFF34D399, 0xFF047857}, {0xFF38BDF8, 0xFF0369A1}}; const char* n[5] = {"Nuit indigo", "Lavande", "Heure dorée", "Forêt", "Marée"};
  for (int i = 0; i < 5; ++i) { Rect r{x + i * 156.f, b.y + 68, 144, 144}; u.p.fill_rrect(r, 20, Paint::linear(145, g[i][0], g[i][1])); u.p.symbol({r.x + 40, r.y + 30, 64, 84}, white(.25f), white(.25f), white(.25f), white(.4f)); u.font.draw(u.p, r.x + 2, r.y + 168, n[i], 14, u.T.fg, Weight::SemiBold); u.font.draw(u.p, r.x + 2, r.y + 186, "Album · 2026", 12, u.T.mut); }
  H(u, x, b.y + 280, "TITRES RÉCENTS"); const char* t[4] = {"Nuit indigo", "Lavande", "Heure dorée", "Marée haute"}; for (int i = 0; i < 4; ++i) list_row(u, {x - 6, b.y + 292 + i * 56.f, b.w - 232, 52}, "music", t[i], "Lumen", i == 0 ? State::Normal : State::Normal, i == 0 ? "3:42" : "3:1" + std::to_string(i), i == 0);
  Rect np{b.x, b.y + b.h - 84, b.w, 84}; u.p.fill_rrect(np, 0, u.T.win_bar); hair(u, np.x, np.y, np.w); u.p.fill_rrect({np.x + 20, np.y + 16, 52, 52}, 14, Paint::linear(145, 0xFFF472B6, 0xFFBE185D)); u.font.draw(u.p, np.x + 86, np.y + 38, "Nuit indigo", 14, u.T.fg, Weight::SemiBold); u.font.draw(u.p, np.x + 86, np.y + 58, "Lumen", 12, u.T.mut);
  float cx = np.x + np.w / 2; u.p.icon("shuffle", {cx - 110, np.y + 16, 20, 20}, u.T.mut); u.p.icon("skip-back", {cx - 66, np.y + 15, 22, 22}, u.T.fg); u.p.fill_rrect({cx - 22, np.y + 8, 40, 40}, 20, Paint::linear(145, u.T.b1, u.T.b2)); u.p.icon("pause", {cx - 14, np.y + 16, 24, 24}, 0xFFFFFFFF); u.p.icon("skip-forward", {cx + 28, np.y + 15, 22, 22}, u.T.fg); u.p.icon("repeat", {cx + 74, np.y + 16, 20, 20}, u.T.mut);
  u.font.draw(u.p, cx - 170, np.y + 70, "1:24", 11, u.T.mut); progress(u, {cx - 140, np.y + 62, 280, 8}, .38f); u.font.draw(u.p, cx + 150, np.y + 70, "3:42", 11, u.T.mut); u.p.icon("volume-2", {np.x + np.w - 190, np.y + 30, 20, 20}, u.T.fg); slider(u, {np.x + np.w - 160, np.y + 28, 130, 24}, .6f);
}
inline void sound(Ctx& c, Rect b) { Ui& u = c.u; float x = b.x + 28, w = b.w - 56; u.font.draw(u.p, x, b.y + 46, "Son", 28, u.T.fg, Weight::SemiBold);
  H(u, x, b.y + 86, "SORTIE"); const char* d[3] = {"Haut-parleurs intégrés", "Casque Bluetooth", "Sortie HDMI"}; const char* di[3] = {"speaker", "headphones", "monitor"}; for (int i = 0; i < 3; ++i) list_row(u, {x - 6, b.y + 98 + i * 56.f, w + 12, 52}, di[i], d[i], i == 0 ? "Appareil par défaut" : "Disponible", State::Normal, i == 0 ? "Actif" : "", i == 0);
  H(u, x, b.y + 282, "VOLUME PRINCIPAL"); u.p.icon("volume-2", {x, b.y + 298, 22, 22}, u.T.fg); slider(u, {x + 36, b.y + 296, w - 110, 24}, .62f); u.font.draw(u.p, x + w - 60, b.y + 313, "62 %", 14, u.T.fg, Weight::SemiBold);
  H(u, x, b.y + 350, "VOLUME PAR APPLICATION"); const char* ap[3] = {"media", "houb", "terminal"}; float v[3] = {.8f, .35f, .0f}; const char* al[3] = {"BMedia", "HOUB", "Notifications"}; for (int i = 0; i < 3; ++i) { const AppDef& a = app(ap[i]); app_tile(u, {x, b.y + 366 + i * 46.f, 34, 34}, a.icon, a.c1, a.c2); u.font.draw(u.p, x + 48, b.y + 388 + i * 46, al[i], 13, u.T.fg, Weight::SemiBold); slider(u, {x + 170, b.y + 366 + i * 46.f + 5, w - 190, 24}, v[i] + .1f); }
  float ex = x + w - 200; for (int i = 0; i < 16; ++i) { float hgt = 8 + 34 * std::fabs(std::sin(i * .7f + 1)); u.p.fill_rrect({ex + i * 12.f, b.y + 76 - hgt + 20, 6, hgt}, 3, Paint::linear(180, u.T.b2, u.T.b1)); }
}
inline void security(Ctx& c, Rect b) { Ui& u = c.u; float x = b.x + 28; ring(u, {x + 90, b.y + 118}, 70, 12, 1.f, u.T.ok, "OK", "Protégé");
  u.font.draw(u.p, x + 200, b.y + 80, "Votre appareil est protégé", 20, u.T.fg, Weight::SemiBold); u.font.draw(u.p, x + 200, b.y + 106, "Dernière analyse : aujourd'hui à 09:12 — aucune menace détectée.", 13, u.T.mut); button(u, {x + 200, b.y + 124, 160, 38}, "Analyser", Kind::Primary, State::Normal, "scan");
  const char* t[6] = {"Pare-feu", "Antivirus", "Mises à jour", "Permissions", "Compte", "Chiffrement"}; const char* s[6] = {"Actif · réseaux privés", "À jour · protection en temps réel", "Une mise à jour disponible", "3 applications surveillées", "Mot de passe fort", "Disque chiffré"}; const char* ic[6] = {"flame", "shield-check", "refresh-cw", "key-round", "user-check", "lock"}; bool warn[6] = {0, 0, 1, 0, 0, 0};
  for (int i = 0; i < 6; ++i) { Rect r{x + (i % 3) * ((b.w - 56) / 3.f), b.y + 214 + (i / 3) * 118.f, (b.w - 56) / 3.f - 12, 106}; card(u, r, 20); u.p.fill_rrect({r.x + 16, r.y + 16, 40, 40}, 13, u.a(warn[i] ? u.T.b4 : u.T.ok, .2f)); u.p.icon(ic[i], {r.x + 24, r.y + 24, 24, 24}, warn[i] ? u.T.b4 : u.T.ok); u.font.draw(u.p, r.x + 16, r.y + 78, t[i], 14, u.T.fg, Weight::SemiBold); u.font.draw(u.p, r.x + 16, r.y + 96, u.font.ellipsize(s[i], r.w - 30, 12), 12, warn[i] ? u.T.b4 : u.T.mut); }
}
inline void update(Ctx& c, Rect b) { Ui& u = c.u; float x = b.x + 40, w = b.w - 80; Rect hero{x, b.y + 24, w, 150}; card(u, hero, 24); u.p.fill_rrect({hero.x + 24, hero.y + 24, 100, 100}, 28, Paint::linear(145, u.T.b1, u.T.b2)); u.p.symbol({hero.x + 52, hero.y + 38, 44, 72}, 0xFFFFFFFF, white(.85f), white(.7f), u.T.b4);
  u.font.draw(u.p, hero.x + 148, hero.y + 52, "BAURA OS 0.2.1", 22, u.T.fg, Weight::SemiBold); u.font.draw(u.p, hero.x + 148, hero.y + 76, "Téléchargement en cours · 64 % · 2 min restantes", 13, u.T.mut); progress(u, {hero.x + 148, hero.y + 92, hero.w - 180, 8}, .64f); button(u, {hero.x + 148, hero.y + 108, 140, 34}, "Suspendre", Kind::Secondary); 
  H(u, x, b.y + 210, "NOUVEAUTÉS"); const char* n[4] = {"Centre d'actions repensé", "Verre plus fluide (-30 % de temps de rendu)", "Navigation au clavier dans tous les menus", "Corrections de sécurité du noyau"}; const char* ic[4] = {"sparkles", "gauge", "keyboard", "shield-check"};
  for (int i = 0; i < 4; ++i) list_row(u, {x - 6, b.y + 222 + i * 56.f, w + 12, 52}, ic[i], n[i], i == 3 ? "Important" : "Amélioration"); 
  H(u, x, b.y + 462, "HISTORIQUE"); text_l(u, x, {x, b.y + 472, w, 28}, "0.2.0 — installée le 2 octobre", 13, u.T.fg); text_l(u, x, {x, b.y + 498, w, 28}, "0.1.4 — installée le 18 septembre", 13, u.T.mut);
}
inline void power(Ctx& c, Rect b) { Ui& u = c.u; float x = b.x + 28; ring(u, {x + 90, b.y + 120}, 72, 12, .84f, u.T.b4, "84 %", "4 h 20 restantes"); u.font.draw(u.p, x + 210, b.y + 70, "Sur batterie", 20, u.T.fg, Weight::SemiBold); u.font.draw(u.p, x + 210, b.y + 94, "Profil d'énergie", 12, u.T.mut, Weight::SemiBold); segmented(u, {x + 210, b.y + 104, 420, 34}, {{"Économie", false}, {"Équilibré", true}, {"Performance", false}});
  toggle(u, {x + 210, b.y + 160, 44, 26}, true); u.font.draw(u.p, x + 266, b.y + 178, "Économiseur de batterie automatique à 20 %", 13, u.T.fg);
  H(u, x, b.y + 230, "CONSOMMATION · 24 H"); graph(u, {x, b.y + 242, b.w - 56, 130}, u.T.b4, 2); H(u, x, b.y + 402, "APPLICATIONS GOURMANDES"); const char* a[3] = {"HOUB", "BMedia", "Serveur d'affichage"}; float v[3] = {.52f, .28f, .14f}; for (int i = 0; i < 3; ++i) { u.font.draw(u.p, x, b.y + 438 + i * 36, a[i], 13, u.T.fg, Weight::SemiBold); progress(u, {x + 200, b.y + 428 + i * 36.f, 280, 8}, v[i]); u.font.draw(u.p, x + 496, b.y + 438 + i * 36, std::to_string(int(v[i] * 100)) + " %", 12, u.T.mut); }
}
inline void network(Ctx& c, Rect b) { Ui& u = c.u; float x = b.x + 28, w = b.w - 56; u.font.draw(u.p, x, b.y + 46, "Réseau", 28, u.T.fg, Weight::SemiBold); toggle(u, {x + w - 54, b.y + 28, 44, 26}, true); text_l(u, x + w - 130, {x, b.y + 28, 0, 26}, "Wi-Fi", 14, u.T.fg, Weight::SemiBold);
  Rect cur{x, b.y + 78, w, 84}; u.p.fill_rrect(cur, 22, Paint::linear(120, u.T.b1, u.T.b2)); u.p.icon("wifi", {cur.x + 24, cur.y + 26, 32, 32}, 0xFFFFFFFF, 2.2f); u.font.draw(u.p, cur.x + 76, cur.y + 38, "Maison-5G", 17, 0xFFFFFFFF, Weight::SemiBold); u.font.draw(u.p, cur.x + 76, cur.y + 60, "Connecté · WPA3 · 866 Mb/s · 192.168.1.24", 12, white(.85f)); Rect dc{cur.x + cur.w - 130, cur.y + 24, 110, 36}; u.p.fill_rrect(dc, 12, white(.22f)); text_c(u, dc, "Déconnecter", 13, 0xFFFFFFFF, Weight::SemiBold);
  H(u, x, b.y + 192, "RÉSEAUX DISPONIBLES"); const char* n[4] = {"Voisin-2.4G", "Café-Libre", "BAURA-Lab", "Réseau masqué"}; const char* ic[4] = {"wifi", "wifi-low", "wifi", "wifi-zero"}; const char* tr[4] = {"Sécurisé", "Ouvert", "Sécurisé", ""};
  for (int i = 0; i < 4; ++i) list_row(u, {x - 6, b.y + 204 + i * 56.f, w + 12, 52}, ic[i], n[i], i == 1 ? "Ouvert · signal moyen" : "WPA2 · signal fort", i == 2 ? State::Hover : State::Normal, tr[i]);
  H(u, x, b.y + 440, "ETHERNET"); list_row(u, {x - 6, b.y + 452, w + 12, 52}, "cable", "Non connecté", "Aucun câble détecté"); button(u, {x, b.y + b.h - 54, 170, 38}, "Réglages avancés", Kind::Secondary, State::Normal, "sliders-horizontal");
}

// ---- Paramètres > Bureau : disposition (la barre coupée en deux est le DÉFAUT ; les autres sont des options), style d'icônes, densité ----
inline void layout_preview(Ui& u, Rect pv, int mode, bool widgets) {   // mode 0 = barre coupée en deux · 1 = pastille flottante · 2 = pleine largeur
  u.p.fill_rrect(pv, 10, Paint::linear(150, 0xFF5A4FD6, 0xFF241C66)); const uint32_t g = 0xB8FFFFFF, gl = 0x70FFFFFF; float by = pv.y + pv.h - 20;
  if (mode == 0) { u.p.fill_rrect({pv.x + 6, by, pv.w * .56f, 14}, 7, g); u.p.fill_rrect({pv.x + pv.w - 6 - pv.w * .24f, by, pv.w * .24f, 14}, 7, g); }
  else if (mode == 1) u.p.fill_rrect({pv.x + pv.w * .2f, by - 2, pv.w * .6f, 16}, 8, g);
  else u.p.fill_rrect({pv.x, pv.y + pv.h - 14, pv.w, 14}, 0, g);
  if (widgets) { u.p.fill_rrect({pv.x + pv.w - 74, pv.y + 6, 44, 10}, 5, gl); u.p.fill_rrect({pv.x + pv.w - 26, pv.y + 6, 20, 10}, 5, gl); }
  u.p.fill_rrect({pv.x + 14, pv.y + 12, 70, 40}, 6, 0x30FFFFFF);
}
inline void desktop_settings(Ctx& c, Rect b, int layout_sel, bool original_icons, bool compact) { Ui& u = c.u;
  sidebar(u, b, 230, {{"palette", "Apparence"}, {"layout-dashboard", "Bureau"}, {"monitor", "Affichage"}, {"wifi", "Réseau"}, {"volume-2", "Son"}, {"lock", "Confidentialité"}, {"user", "Comptes"}, {"languages", "Langue et région"}, {"accessibility", "Accessibilité"}}, 1);
  float x = b.x + 262, w = b.w - 294; u.font.draw(u.p, x, b.y + 44, "Bureau", 28, u.T.fg, Weight::SemiBold);
  H(u, x, b.y + 84, "DISPOSITION"); const char* nm[3] = {"Barre coupée en deux", "Pastille flottante", "Pleine largeur"}; const char* sub[3] = {"Par défaut · BAURA", "Option", "Option"};
  for (int i = 0; i < 3; ++i) { Rect t{x + i * 232.f, b.y + 96, 220, 176}; card(u, t); if (i == layout_sel) u.p.stroke_rrect(t, 18, 2.5f, u.T.b1); layout_preview(u, {t.x + 12, t.y + 12, t.w - 24, 104}, i, true);
    u.font.draw(u.p, t.x + 14, t.y + 140, nm[i], 14, u.T.fg, Weight::SemiBold); u.font.draw(u.p, t.x + 14, t.y + 160, sub[i], 12, i == 0 ? u.T.b4 : u.T.mut, i == 0 ? Weight::SemiBold : Weight::Regular); if (i == layout_sel) u.p.icon("circle-check", {t.x + t.w - 34, t.y + 130, 20, 20}, u.T.b1, 2.2f); }
  H(u, x, b.y + 306, "STYLE DES ICÔNES"); const char* ids[6] = {"files", "houb", "terminal", "media", "store", "settings"};
  for (int i = 0; i < 2; ++i) { Rect t{x + i * 348.f, b.y + 318, 336, 124}; card(u, t); bool on = (i == 1) == original_icons; if (on) u.p.stroke_rrect(t, 18, 2.5f, u.T.b1);
    bool keep = baura_icons_enabled(); baura_icons_enabled() = (i == 0); for (int k = 0; k < 6; ++k) { Rect ir{t.x + 14 + k * 52.f, t.y + 14, 40, 40}; app_tile(u, ir, app(ids[k]).icon, app(ids[k]).c1, app(ids[k]).c2); } baura_icons_enabled() = keep;
    u.font.draw(u.p, t.x + 14, t.y + 86, i == 0 ? "Icônes BAURA" : "Icônes d'origine", 14, u.T.fg, Weight::SemiBold); u.font.draw(u.p, t.x + 14, t.y + 106, i == 0 ? "Dessinées pour BAURA OS, cohérentes partout" : "Icônes fournies par chaque application", 12, u.T.mut); if (on) u.p.icon("circle-check", {t.x + t.w - 34, t.y + 84, 20, 20}, u.T.b1, 2.2f); }
  H(u, x, b.y + 476, "DENSITÉ"); segmented(u, {x, b.y + 488, 300, 34}, {{"Confortable", !compact}, {"Compacte", compact}});
  list_row(u, {x - 6, b.y + 530, w + 12, 52}, "layout-grid", "Widgets en haut à droite", "Lecteur multimédia, météo et notifications", State::Normal, "", true);
}
}  // namespace BauraApps
#endif
