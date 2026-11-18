// Test 08 : arabe (jonction + bidi), miroir RTL, contraste WCAG, contraste élevé, échelle de texte 150 %, arbre d'accessibilité + focus.
#include "baura_bidi.h"
#include "baura_a11y.h"
#include "bmp_out.h"
#include <cstdio>
#include <cstring>
using namespace BauraUI; using namespace BauraText; using namespace BauraA11y;
#define CHECK(c) do { if (!(c)) { std::printf("ECHEC ligne %d : %s\n", __LINE__, #c); return 1; } } while (0)
struct Canvas { int W, H; std::vector<uint32_t> px; BGEFramebuffer fb; BGERenderer r; Painter p; Canvas(int w, int h) : W(w), H(h), px(size_t(w) * h), fb{px.data(), uint32_t(w), uint32_t(h), uint32_t(w), px.size()}, r(fb), p(r) {} };
static std::string cps(const U32& v) { std::string s; char b[16]; for (auto c : v) { std::snprintf(b, 16, "%04X ", c); s += b; } return s; }

// Texte orienté : x = bord de départ du conteneur (gauche en LTR, droite en RTL) ; dessine à partir du bord de lecture.
static float txt(Ui& u, const Font& f, Rect box, const std::string& s, int px, uint32_t col, Weight w = Weight::Regular, float inset = 0, float baseline_dy = 0) {
  std::string v = u.rtl ? visual(s, true) : s; float tw = f.measure(v, px, w); auto m = f.metrics(px, w); float y = box.y + (box.h + m.ascent - m.descent) / 2 - 1 + baseline_dy;
  float x = u.rtl ? box.x + box.w - inset - tw : box.x + inset; f.draw(u.p, x, y, v, px, col, w); return tw;
}
struct Labels { const char* title; const char* items[5]; const char* theme[3]; const char* head_theme; const char* t1; const char* t2; const char* win; };
static void settings_demo(Ui& u, const Font& f, Rect r, const Labels& L) {
  const auto& T = u.T; bool rtl = u.rtl; Painter& p = u.p;
  shadow(p, r, 22, 24, 56, 0x59140D40); GlassStyle g; g.tint = T.win; g.line = T.win_line; g.highlight = T.highlight; glass(p, r, 22, g);
  p.set_clip(int(r.x), int(r.y), int(r.w), 46); p.fill_rrect({r.x, r.y, r.w, 46 + 22}, 22, T.win_bar); p.reset_clip(); p.fill_rrect({r.x, r.y + 45, r.w, 1}, 0, T.hair);
  Rect ic{rtl ? r.x + r.w - 14 - 22 : r.x + 14, r.y + 12, 22, 22}; p.fill_rrect(ic, 7, Paint::linear(145, 0xFF9AA0C8, 0xFF6B7199)); p.icon("settings", ic.inset(4), 0xFFFFFFFF);
  txt(u, f, {r.x, r.y, r.w, 46}, L.win, 14, T.fg, Weight::SemiBold, 46, 0);
  const char* ci[3] = {"x", "square", "minus"}; for (int i = 0; i < 3; ++i) { Rect b{rtl ? r.x + 14 + i * 38.f : r.x + r.w - 14 - 32 - i * 38.f, r.y + 11, 32, 24}; p.fill_rrect(b, 9, i == 0 ? u.a(T.b4, .22f) : u.a(T.b3, .35f)); p.icon(ci[i], {b.x + 9, b.y + 5, 14, 14}, T.fg, 2.2f); }
  Rect sb{rtl ? r.x + r.w - 190 : r.x, r.y + 46, 190, r.h - 46}; p.fill_rrect(sb, 0, u.a(T.b3, .10f)); p.fill_rrect({rtl ? sb.x : sb.x + sb.w - 1, sb.y, 1, sb.h}, 0, T.hair);
  const char* ico[5] = {"palette", "monitor", "wifi", "volume-2", "lock"};
  for (int i = 0; i < 5; ++i) { Rect it{sb.x + 10, sb.y + 12 + i * 40.f, sb.w - 20, 36}; if (i == 0) { p.fill_rrect(it, 12, u.a(T.b1, .16f)); p.fill_rrect({rtl ? it.x + it.w - 3 : it.x, it.y + 9, 3, 18}, 1.5f, T.b1); }
    p.icon(ico[i], {rtl ? it.x + it.w - 32 : it.x + 12, it.y + 8, 20, 20}, i == 0 ? T.accent : T.fg); txt(u, f, it, L.items[i], 14, i == 0 ? T.accent : T.fg, i == 0 ? Weight::SemiBold : Weight::Regular, 42); }
  Rect c{rtl ? r.x : r.x + 190, r.y + 46, r.w - 190, r.h - 46};
  txt(u, f, {c.x, c.y + 16, c.w, 36}, L.items[0], 28, T.fg, Weight::SemiBold, 28);
  txt(u, f, {c.x, c.y + 62, c.w, 20}, L.head_theme, 12, T.mut, Weight::SemiBold, 28);
  for (int i = 0; i < 3; ++i) { float w = 108, gap = 12; float off = 28 + i * (w + gap); Rect t{rtl ? c.x + c.w - off - w : c.x + off, c.y + 88, w, 96}; u.p.fill_rrect(t, 16, u.T.field); u.p.stroke_rrect(t, 16, i == 0 ? 2.5f : 1.f, i == 0 ? T.b1 : T.field_line);
    p.fill_rrect({t.x + 10, t.y + 10, t.w - 20, 44}, 10, i == 1 ? Paint::linear(150, 0xFF1A1450, 0xFF3B1F8F) : Paint::linear(150, 0xFFF1E4FF, 0xFFA99BFF)); txt(u, f, {t.x, t.y + 60, t.w, 28}, L.theme[i], 13, T.fg, Weight::SemiBold, 12); }
  for (int i = 0; i < 2; ++i) { Rect row{c.x + 20, c.y + 206 + i * 54.f, c.w - 40, 48}; p.fill_rrect(row, 16, T.field); p.stroke_rrect(row, 16, 1, T.field_line);
    txt(u, f, row, i ? L.t2 : L.t1, 14, T.fg, Weight::SemiBold, 18); bool on = i == 0; Rect tg{rtl ? row.x + 16 : row.x + row.w - 16 - 44, row.y + 11, 44, 26};
    p.fill_rrect(tg, 13, on ? Paint::linear(90, T.b1, T.b2) : Paint(T.hair)); float kx = on ? (rtl ? tg.x + 3 : tg.x + tg.w - 23) : (rtl ? tg.x + tg.w - 23 : tg.x + 3); shadow(p, {kx, tg.y + 3, 20, 20}, 10, 1, 3, 0x40000000); p.fill_rrect({kx, tg.y + 3, 20, 20}, 10, 0xFFFFFFFF); }
}
int main() {
  // ============ 1. Jonction ============
  { U32 s = shape(utf8_decode("سلام")); CHECK(s.size() == 3 && s[0] == 0xFEB3 && s[1] == 0xFEFC && s[2] == 0xFEE1); }       // seen initiale, lam-alef finale, meem isolée
  { U32 s = shape(utf8_decode("محمد")); CHECK(s.size() == 4 && s[0] == 0xFEE3 && s[1] == 0xFEA4 && s[2] == 0xFEE4 && s[3] == 0xFEAA); }
  { U32 s = shape(utf8_decode("الم")); CHECK(s.size() == 3 && s[0] == 0xFE8D && s[1] == 0xFEDF && s[2] == 0xFEE2); }                           // alef isolé (ne joint pas à gauche), lam initiale, meem finale
  { U32 s = shape(utf8_decode("لا")); CHECK(s.size() == 1 && s[0] == 0xFEFB); }                                                 // lam-alef isolée
  { U32 s = shape(utf8_decode("بلا")); CHECK(s.size() == 2 && s[0] == 0xFE91 && s[1] == 0xFEFC); }                              // ba initiale + lam-alef finale
  { U32 s = shape(utf8_decode("سَلام")); CHECK(s.size() == 3); }                                                               // fatha ignorée
  { U32 s = shape(utf8_decode("abc ١٢")); CHECK(s.size() == 6); }                                                              // non arabe inchangé
  // ============ 2. Bidi ============
  CHECK(visual("Wi-Fi", true) == "Wi-Fi"); CHECK(visual("BAURA OS 0.2", false) == "BAURA OS 0.2");
  { std::string v = visual("إصدار BAURA OS 0.2 جاهز", true); size_t a = v.find("BAURA OS 0.2"); CHECK(a != std::string::npos);                    // le bloc latin garde son ordre interne
    std::string ready = visual("جاهز", true), rel = visual("إصدار", true); size_t pr = v.find(ready), pl = v.find(rel); CHECK(pr == 0 && pl > a && a > pr); }   // visuel G→D : « جاهز », bloc latin, « إصدار »
  { std::string v = visual("(سلام)", true); CHECK(v.front() == '(' && v.back() == ')'); }                                       // parenthèses miroir
  { std::string v = visual("12:30 سلام", true); CHECK(v.find("12:30") != std::string::npos && v.find("12:30") > 0); }          // l'heure n'est pas inversée
  CHECK(std::strcmp(rtl_icon("chevron-right", true), "chevron-left") == 0 && std::strcmp(rtl_icon("check", true), "check") == 0 && std::strcmp(rtl_icon("chevron-right", false), "chevron-right") == 0);
  CHECK(start_x(10, 100, 30, true) == 80 && start_x(10, 100, 30, false) == 10);
  // ============ 3. Contraste ============
  CHECK(std::fabs(contrast(0xFFFFFFFF, 0xFF000000) - 21.f) < .01f); CHECK(std::fabs(contrast(0xFF777777, 0xFFFFFFFF) - 4.48f) < .02f); CHECK(std::fabs(contrast(0xFF767676, 0xFFFFFFFF) - 4.54f) < .02f);
  // ============ 4. Arbre + focus ============
  Node dlg; dlg.role = Role::Dialog; dlg.name = "Autoriser cette application ?";
  auto mk = [](Role r, const char* n, Rect rc, bool foc, bool dis = false) { Node x; x.role = r; x.name = n; x.rect = rc; x.focusable = foc; x.disabled = dis; return x; };
  dlg.kids.push_back(mk(Role::Label, "Elle demande des droits d'administrateur.", {0, 0, 0, 0}, false));
  dlg.kids.push_back(mk(Role::TextField, "Mot de passe", {28, 150, 400, 42}, true));
  { Node g = mk(Role::RadioGroup, "Durée", {28, 205, 400, 30}, false); g.kids = {mk(Role::Radio, "Une fois", {28, 205, 120, 30}, true), mk(Role::Radio, "15 minutes", {150, 205, 120, 30}, true), mk(Role::Radio, "Toujours", {272, 205, 120, 30}, true, true)}; g.kids[0].checked = true; dlg.kids.push_back(g); }
  dlg.kids.push_back(mk(Role::Button, "Refuser", {240, 262, 192, 40}, true)); dlg.kids.push_back(mk(Role::Button, "Autoriser", {28, 262, 200, 40}, true));
  FocusManager fm; fm.rebuild(dlg); CHECK(fm.order.size() == 5);                                                                  // champ, 2 radios (le 3e est indisponible), 2 boutons
  CHECK(fm.tab(false)->name == "Mot de passe"); CHECK(fm.tab(false)->name == "Une fois"); CHECK(fm.tab(false)->name == "15 minutes"); CHECK(fm.tab(false)->name == "Refuser"); CHECK(fm.tab(false)->name == "Autoriser"); CHECK(fm.tab(false)->name == "Mot de passe");   // boucle
  CHECK(fm.tab(true)->name == "Autoriser"); CHECK(fm.tab(true)->name == "Refuser");
  fm.cur = 1; const Node& grp = dlg.kids[2]; CHECK(fm.arrow(+1, false, grp)->name == "15 minutes"); CHECK(fm.arrow(+1, false, grp)->name == "Une fois");   // flèche droite en LTR, boucle (l'indisponible est sauté)
  CHECK(fm.arrow(+1, true, grp)->name == "15 minutes");                                                                          // en RTL la flèche droite RECULE : « Une fois » -> « 15 minutes » (boucle arrière)
  { dlg.kids[3].hidden = true; fm.rebuild(dlg); CHECK(fm.order.size() == 4); }
  CHECK(announce(dlg.kids[2].kids[0]) == "Une fois, radio, activé"); CHECK(announce(dlg.kids[2].kids[2]).find("indisponible") != std::string::npos);
  dlg.kids[3].hidden = false; fm.rebuild(dlg);

  auto blob = read_file("../../common/inter.bgf"); Font inter; inter.load(blob.data(), blob.size()); auto ab = read_file("../../common/arabic.bgf"); Font arab; arab.load(ab.data(), ab.size());
  // ============ Rendu A : LTR / RTL ============
  { Canvas c(1300, 560); const auto& T = BauraBrand::LIGHT; Ui ul{c.p, inter, T}; c.p.fill_rrect({0, 0, 1300, 560}, 0, Paint::linear(150, 0xFFF1E4FF, 0xFFA99BFF));
    Labels fr{"Paramètres", {"Apparence", "Affichage", "Réseau", "Son", "Confidentialité"}, {"Clair", "Sombre", "Auto"}, "THÈME", "Transparence du verre", "Animations", "Paramètres"};
    Labels ar{"الإعدادات", {"المظهر", "العرض", "الشبكة", "الصوت", "الخصوصية"}, {"فاتح", "داكن", "تلقائي"}, "السمة", "شفافية الزجاج", "الحركات", "الإعدادات"};
    settings_demo(ul, inter, {30, 40, 600, 470}, fr); Ui ur{c.p, arab, T}; ur.rtl = true; settings_demo(ur, arab, {670, 40, 600, 470}, ar);
    inter.draw(c.p, 34, 30, "LTR — français", 12, T.mut, Weight::SemiBold); arab.draw(c.p, 674, 30, ("RTL — " + visual("العربية", true) + " (miroir complet)"), 12, T.mut, Weight::SemiBold); write_ppm("rtl_comparaison.ppm", c.px, c.W, c.H); }
  // Rendu A2 : texte arabe — avant/après jonction
  { Canvas c(1000, 330); const auto& T = BauraBrand::LIGHT; c.p.fill_rrect({0, 0, 1000, 330}, 0, 0xFFF7F3FF);
    const char* ex[3] = {"سلام", "محمد", "إصدار BAURA OS 0.2 جاهز"}; float y = 56;
    arab.draw(c.p, 24, 24, "Sans traitement (lettres isolées, ordre logique)  →  Avec baura_bidi.h", 12, T.mut, Weight::SemiBold);
    for (int i = 0; i < 3; ++i) { std::string raw; { U32 d = utf8_decode(ex[i]); raw = utf8_encode(d); }
      // « avant » : caractères de base 06xx tels quels, dessinés de gauche à droite (faux)
      arab.draw(c.p, 24, y + 40, raw, 28, 0xFFD6303C); std::string v = visual(ex[i], true); float tw = arab.measure(v, 28); arab.draw(c.p, 976 - tw, y + 40, v, 28, T.fg);
      arab.draw(c.p, 24, y + 70, i < 2 ? cps(shape(utf8_decode(ex[i]))) : "bloc latin « BAURA OS 0.2 » conservé, phrase lue de droite à gauche", 12, T.mut); y += 88; }
    write_ppm("arabe_jonction.ppm", c.px, c.W, c.H); }
  // ============ Rendu B : thèmes d'accessibilité ============
  struct TH { const BauraBrand::Tokens* t; const char* name; uint32_t bg; bool hc; } ths[4] = {{&BauraBrand::LIGHT, "Clair", 0xFFD9CCFF, false}, {&BauraBrand::DARK, "Sombre", 0xFF2A1A78, false}, {&HC_LIGHT, "Contraste élevé — clair", 0xFFFFFFFF, true}, {&HC_DARK, "Contraste élevé — sombre", 0xFF000000, true}};
  { Canvas c(1200, 760); for (int i = 0; i < 4; ++i) { const auto& T = *ths[i].t; Ui u{c.p, inter, T}; float ox = (i % 2) * 600.f, oy = (i / 2) * 380.f; c.p.fill_rrect({ox, oy, 600, 380}, 0, ths[i].bg);
      inter.draw(c.p, ox + 24, oy + 30, ths[i].name, 14, ths[i].hc ? T.fg : (i == 1 ? 0xFFF4F2FF : 0xFF17133D), Weight::SemiBold);
      Rect card{ox + 24, oy + 48, 552, 310}; if (ths[i].hc) { c.p.fill_rrect(card, 16, T.win); c.p.stroke_rrect(card, 16, 2, T.fg); } else panel(u, card, 22, true);
      float x = card.x + 20, y = card.y + 20; button(u, {x, y, 150, 38}, "Continuer", Kind::Primary); button(u, {x + 162, y, 130, 38}, "Annuler", Kind::Secondary); button(u, {x + 304, y, 120, 38}, "Supprimer", Kind::Danger);
      if (ths[i].hc) { Rect fr{x - 4, y - 4, 158, 46}; c.p.stroke_rrect(fr, 14, 3, T.b4); }
      Rect in{x, y + 62, 300, 42}; input(u, in, "ahmed@baura", "", State::Focus, "user"); if (ths[i].hc) { c.p.stroke_rrect({in.x - 3, in.y - 3, in.w + 6, in.h + 6}, 15, 3, T.b4); }
      toggle(u, {x + 330, y + 70, 44, 26}, true); checkbox(u, {x + 392, y + 73, 20, 20}, true); radio(u, {x + 424, y + 73, 20, 20}, true); chip(u, {x + 456, y + 70, 80, 26}, "Récents", true);
      list_row(u, {x - 6, y + 126, 530, 52}, "folder", "Documents", "12 éléments", State::Normal, "hier", true); list_row(u, {x - 6, y + 184, 530, 52}, "image", "Images", "48 éléments", State::Hover, "lun.");
      u.font.draw(c.p, x, y + 262, "Texte secondaire : 12 px, ratio ≥ 4,5 : 1 exigé", 12, T.mut); progress(u, {x, y + 276, 300, 8}, .6f); }
    write_ppm("a11y_themes.ppm", c.px, c.W, c.H); }
  // ============ Rendu C : échelle de texte 100 % / 150 % ============
  { Canvas c(1240, 520); const auto& T = BauraBrand::LIGHT; Ui u{c.p, inter, T}; c.p.fill_rrect({0, 0, 1240, 520}, 0, Paint::linear(150, 0xFFF1E4FF, 0xFFA99BFF));
    for (int k = 0; k < 2; ++k) { float s = k ? 1.5f : 1.f; float ox = 24 + k * 580.f; Rect card{ox, 40, 560, 440}; panel(u, card, 24, true); inter.draw(c.p, ox, 28, k ? "Échelle du texte : 150 %" : "Échelle du texte : 100 %", 12, 0xFF17133D, Weight::SemiBold);
      const char* t[3] = {"Documents", "Téléchargements récents", "Corbeille"}; const char* sb[3] = {"12 éléments", "48 éléments · modifié hier", "Vide"}; float y = card.y + 18;
      for (int i = 0; i < 3; ++i) { float h = 52 * s, ic = 36 * s; Rect row{card.x + 12, y, card.w - 24, h}; if (i == 0) c.p.fill_rrect(row, 14 * s, u.a(T.b1, .14f)); c.p.fill_rrect({row.x + 10, row.y + (h - ic) / 2, ic, ic}, 11 * s, i == 0 ? Paint::linear(145, T.b1, T.b2) : Paint(T.hair)); c.p.icon(i == 1 ? "download" : i ? "trash-2" : "folder", {row.x + 10 + ic * .2f, row.y + (h - ic) / 2 + ic * .2f, ic * .6f, ic * .6f}, i == 0 ? 0xFFFFFFFF : T.fg);
        int tp = int(14 * s + .5f), sp = int(12 * s + .5f); auto mt = inter.metrics(tp, Weight::SemiBold); float tx = row.x + 10 + ic + 12; float room = row.w - (tx - row.x) - 12;
        inter.draw(c.p, tx, row.y + h / 2 - 3, inter.ellipsize(t[i], room, tp, Weight::SemiBold), tp, T.fg, Weight::SemiBold); inter.draw(c.p, tx, row.y + h / 2 + sp + 1, inter.ellipsize(sb[i], room, sp), sp, T.mut); (void)mt; y += h + 6; }
      float bh = 38 * s; Rect b1{card.x + 20, y + 14, 150 * s, bh}; c.p.fill_rrect(b1, 12 * s, Paint::linear(145, T.b1, T.b2)); int bp = int(14 * s + .5f); float bw = inter.measure("Continuer", bp, Weight::SemiBold); inter.draw(c.p, b1.x + (b1.w - bw) / 2, b1.y + b1.h / 2 + bp * .35f, "Continuer", bp, 0xFFFFFFFF, Weight::SemiBold);
      Rect fld{card.x + 20, y + 14 + bh + 16, card.w - 40, 42 * s}; c.p.fill_rrect(fld, 12 * s, T.field); c.p.stroke_rrect(fld, 12 * s, 1.5f, T.b1); inter.draw(c.p, fld.x + 14, fld.y + fld.h / 2 + bp * .35f, "Rechercher un fichier ou un réglage…", bp, T.mut); }
    write_ppm("a11y_echelle.ppm", c.px, c.W, c.H); }
  // ============ Rendu D : ordre de tabulation ============
  { Canvas c(860, 470); const auto& T = BauraBrand::LIGHT; Ui u{c.p, inter, T}; c.p.fill_rrect({0, 0, 860, 470}, 0, Paint::linear(150, 0xFFF1E4FF, 0xFFA99BFF)); Rect r{200, 40, 460, 330}; panel(u, r, 26, true);
    c.p.fill_rrect({r.x, r.y, r.w, 6}, 0, T.b4); inter.draw(c.p, r.x + 28, r.y + 50, "Autoriser cette application ?", 17, T.fg, Weight::SemiBold); inter.draw(c.p, r.x + 28, r.y + 76, "Elle demande des droits d'administrateur.", 13, T.mut);
    auto off = [&](Rect q) { return Rect{q.x + r.x, q.y + r.y - 40 + 40, q.w, q.h}; };
    input(u, off({28, 100 + 10, 404, 42}), "", "Mot de passe", State::Normal, "key-round"); const char* rl[3] = {"Une fois", "15 minutes", "Toujours"};
    for (int i = 0; i < 3; ++i) { Rect q = off({28.f + i * 134, 166, 126, 30}); radio(u, {q.x, q.y + 5, 20, 20}, i == 0, i == 2 ? State::Disabled : State::Normal); inter.draw(c.p, q.x + 28, q.y + 20, rl[i], 14, i == 2 ? u.a(T.fg, .4f) : T.fg); }
    button(u, off({28, 222, 200, 40}), "Autoriser", Kind::Primary); button(u, off({240, 222, 192, 40}), "Refuser", Kind::Secondary);
    // positions réelles = rectangles du modèle : on recalcule l'ordre depuis FocusManager et on pose les pastilles
    Node d2; d2.role = Role::Dialog; d2.kids = {mk(Role::TextField, "Mot de passe", off({28, 110, 404, 42}), true), mk(Role::Radio, "Une fois", off({28, 166, 126, 30}), true), mk(Role::Radio, "15 minutes", off({162, 166, 126, 30}), true), mk(Role::Radio, "Toujours", off({296, 166, 126, 30}), true, true), mk(Role::Button, "Autoriser", off({28, 222, 200, 40}), true), mk(Role::Button, "Refuser", off({240, 222, 192, 40}), true)};
    FocusManager f2; f2.rebuild(d2); int n = 0; for (auto* nd : f2.order) { ++n; Rect b{nd->rect.x - 14, nd->rect.y - 16, 24, 24}; c.p.fill_rrect(b, 12, T.b4); c.p.stroke_rrect(b, 12, 2, 0xFFFFFFFF); text_c(u, b, std::to_string(n), 12, 0xFFFFFFFF, Weight::SemiBold); }
    f2.cur = 0; focus_ring(u, f2.order[0]->rect, 12); inter.draw(c.p, 24, 28, "Pastilles = ordre de tabulation calculé par FocusManager ; « Toujours » (indisponible) est sauté", 12, T.mut, Weight::SemiBold);
    write_ppm("a11y_ordre-focus.ppm", c.px, c.W, c.H); std::string tree; dump(dlg, tree); FILE* fo = std::fopen("arbre_a11y.txt", "w"); std::fputs(tree.c_str(), fo); std::fclose(fo); }
  // ============ Table de contraste (Markdown) ============
  FILE* md = std::fopen("contraste.md", "w"); std::fprintf(md, "| Thème | Paire | Ratio (pire fond) | Seuil | Résultat | Correctif proposé |\n|---|---|---|---|---|---|\n");
  int fails = 0; for (int d = 0; d < 2; ++d) { const auto& T = d ? BauraBrand::DARK : BauraBrand::LIGHT; uint32_t w0 = d ? 0xFF1A1450 : 0xFFF1E4FF, w1 = d ? 0xFF3B1F8F : 0xFFA99BFF; uint32_t surf = T.win;
    struct Pair { const char* n; uint32_t fg; float need; bool darken; } ps[] = {{"texte principal (fg) / fenêtre", T.fg, 4.5f, !d}, {"texte discret (mut) / fenêtre", T.mut, 4.5f, !d}, {"accent (liens, onglet actif)", T.accent, 4.5f, !d}, {"or (badge Administrateur)", T.b4, 4.5f, !d}, {"erreur (err)", T.err, 4.5f, !d}, {"succès (ok)", T.ok, 4.5f, !d}, {"bord de champ / fond", T.field_line, 3.f, !d}, {"indigo (b1) : composants, filets", T.b1, 3.f, !d}};
    for (auto& p : ps) { float r = worst(p.fg, surf, w0, w1); bool okk = r >= p.need; if (!okk) ++fails; char fx[64] = "—"; if (!okk) { uint32_t g = fix(p.fg, surf, w0, w1, p.need, p.darken); std::snprintf(fx, 64, "#%06X (%.1f : 1)", g & 0xFFFFFF, worst(g, surf, w0, w1)); }
      std::fprintf(md, "| %s | %s | %.2f : 1 | %.1f : 1 | %s | %s |\n", d ? "sombre" : "clair", p.n, r, p.need, okk ? "OK" : "**insuffisant**", fx); }
    uint32_t btn[3] = {T.b1, T.b2, T.err}; const char* bn[3] = {"texte blanc / bouton primaire (b1)", "texte blanc / violet (b2)", "texte blanc / bouton danger (err)"}; for (int i = 0; i < 3; ++i) { float r = contrast(0xFFFFFFFF, btn[i]); if (r < 4.5f) ++fails; std::fprintf(md, "| %s | %s | %.2f : 1 | 4.5 : 1 | %s | — |\n", d ? "sombre" : "clair", bn[i], r, r >= 4.5f ? "OK" : "**insuffisant**"); } }
  for (int h = 0; h < 2; ++h) { const auto& T = h ? HC_DARK : HC_LIGHT; float a = contrast(T.fg, T.win), b = contrast(T.mut, T.win), c2 = contrast(T.b4, T.win); CHECK(a >= 7.f && b >= 7.f && c2 >= 7.f);
    std::fprintf(md, "| contraste élevé %s | fg / mut / focus sur fond | %.1f / %.1f / %.1f : 1 | 7 : 1 (AAA) | OK | — |\n", h ? "sombre" : "clair", a, b, c2); }
  std::fclose(md); std::printf("08 : tests passent ; %d paires sous le seuil (voir contraste.md)\n", fails); return 0;
}
