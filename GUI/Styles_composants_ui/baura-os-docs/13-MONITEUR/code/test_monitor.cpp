// Rendus du Moniteur BAURA (clair/sombre) alimentés par le BACKEND RUST (source synthétique déterministe).
#include <algorithm>
#include <cstdio>
#include "baura_monitor_view.h"
#include "bmp_out.h"
using namespace BauraMonitor;
static int fails = 0;
#define CHECK(c) do { if (!(c)) { std::printf("ECHEC: %s (ligne %d)\n", #c, __LINE__); ++fails; } } while (0)

static void render(BauraSvc::Services& sv, const Font& font, bool dark, Page page, int selected, bool confirm, const char* out) {
  const int W = 1280, H = 780; std::vector<uint32_t> px(size_t(W) * H); BGEFramebuffer fb{px.data(), W, H, W, px.size()}; BGERenderer rd(fb); Painter p(rd);
  const auto& T = dark ? BauraBrand::DARK : BauraBrand::LIGHT; Ui u{p, font, T};
  p.fill_rrect({0, 0, float(W), float(H)}, 0, dark ? Paint::linear(150, 0xFF241C66, 0xFF0A0820) : Paint::linear(150, 0xFFF7F1FF, 0xFFCFC6FF));
  Rect win{50, 30, 1180, 720}; Rect b = window_frame(u, win, "Moniteur", "monitor", T.b1, T.b2);
  BauraVec::appicons::Style nost; nost.shadow = false;   // le titre de fenêtre garde l'ancienne pastille ; l'icône BAURA complète sert dans Démarrer/Barre
  BauraSvc::Layout dummy = sv.layout(); (void)dummy;
  auto snap = sv.snapshot(); auto groups = sv.groups(); float mem_pct = 100.f * snap.mem_used_kib / snap.mem_total_kib;
  Rect tabs{b.x + 20, b.y + 14, 440, 34}; int cur = int(page);
  segmented(u, tabs, {{"Vue d'ensemble", cur == 0}, {"Applications", cur == 1}, {"Performances", cur == 2}});
  u.font.draw(p, b.x + b.w - 20 - font.measure("Mis à jour à l'instant", 12), b.y + 36, "Mis à jour à l'instant", 12, T.mut);
  float cx = b.x + 20, cy = b.y + 64, cw = b.w - 40;
  if (page == Page::Overview) {
    verdict(u, {cx, cy, cw, 80}, groups.empty() ? nullptr : &groups[0], snap.cpu_total_pct, mem_pct);
    float w = (cw - 3 * 16) / 4, y = cy + 96, h = 168;
    stat_card(u, {cx, y, w, h}, {"Processeur", "cpu", fmt1(snap.cpu_total_pct) + " %", std::to_string(snap.cores) + " cœurs", snap.cpu_total_pct, T.b1, T.b2, sv.history(0), 100});
    stat_card(u, {cx + (w + 16), y, w, h}, {"Mémoire", "memory-stick", fmt1(mem_pct) + " %", bytes_kib(snap.mem_used_kib) + " / " + bytes_kib(snap.mem_total_kib), mem_pct, T.b2, T.b3, sv.history(1), 100});
    uint64_t nb = snap.net_rx_bps + snap.net_tx_bps, db = snap.disk_read_bps + snap.disk_write_bps;
    stat_card(u, {cx + 2 * (w + 16), y, w, h}, {"Réseau", "wifi", rate(snap.net_rx_bps), "↑ " + rate(snap.net_tx_bps), std::min(99.f, nb / 50000.f), T.ok, T.b1, sv.history(2), 0});
    stat_card(u, {cx + 3 * (w + 16), y, w, h}, {"Disque", "hard-drive", rate(snap.disk_read_bps), "écriture " + rate(snap.disk_write_bps), std::min(99.f, db / 1200000.f), T.b4, T.err, sv.history(3), 0});
    // cœurs
    Rect cr{cx, y + h + 16, (cw - 16) * .46f, 300}; panel(u, cr, 22); p.icon("activity", {cr.x + 18, cr.y + 16, 18, 18}, T.mut, 2.1f); u.font.draw(p, cr.x + 42, cr.y + 30, "Chaque cœur du processeur", 13, T.mut, Weight::SemiBold);
    auto cores = sv.cores(); for (size_t i = 0; i < cores.size(); ++i) { float yy = cr.y + 52 + i * 30.f; u.font.draw(p, cr.x + 20, yy + 14, "Cœur " + std::to_string(i + 1), 12, T.mut);
      Rect tr{cr.x + 84, yy + 4, cr.w - 84 - 70, 12}; p.fill_rrect(tr, 6, T.hair); p.fill_rrect({tr.x, tr.y, std::max(12.f, tr.w * cores[i] / 100), tr.h}, 6, Paint::linear(90, T.b1, cores[i] > 80 ? T.err : T.b2)); u.font.draw(p, tr.x + tr.w + 12, yy + 14, fmt1(cores[i]) + " %", 12, T.fg, Weight::SemiBold); }
    // top applications
    Rect ar{cr.x + cr.w + 16, cr.y, cw - cr.w - 16, 300}; panel(u, ar, 22); p.icon("layout-grid", {ar.x + 18, ar.y + 16, 18, 18}, T.mut, 2.1f); u.font.draw(p, ar.x + 42, ar.y + 30, "Ce qui travaille le plus", 13, T.mut, Weight::SemiBold);
    for (int i = 0; i < 4 && i < int(groups.size()); ++i) { Rect rr{ar.x + 10, ar.y + 46 + i * 62.f, ar.w - 20, 58}; app_row(u, rr, groups[i], false, false, false, true); }
  } else if (page == Page::Apps) {
    Rect hd{cx, cy, cw, 38}; u.font.draw(p, cx + 64, cy + 24, "Application", 12, T.mut, Weight::SemiBold); float x = cx + cw - 470 - 18; u.font.draw(p, x, cy + 24, "Processeur", 12, T.mut, Weight::SemiBold); u.font.draw(p, x + 100, cy + 24, "Mémoire", 12, T.mut, Weight::SemiBold); u.font.draw(p, x + 200, cy + 24, "Impact", 12, T.mut, Weight::SemiBold); (void)hd;
    Rect list{cx - 6, cy + 36, cw + 12, b.h - 64 - 36 - 62}; p.set_clip(int(list.x), int(list.y), int(list.w), int(list.h)); float y = list.y;
    for (size_t i = 0; i < groups.size(); ++i) { Rect rr{cx, y, cw, 58}; bool sel = int(i) == selected; Rect bt = app_row(u, rr, groups[i], false, sel, sel && confirm); (void)bt; y += 62; }
    p.reset_clip(); { float total = groups.size() * 62.f; scrollbar(u, {list.x + list.w - 10, list.y, 10, list.h}, 0.f, std::min(1.f, list.h / total)); }
    Rect foot{cx, b.y + b.h - 54, cw, 36}; u.p.icon("info", {foot.x + 4, foot.y + 9, 18, 18}, T.mut, 2); u.font.draw(p, foot.x + 30, foot.y + 23, "Les applications sont triées par impact : les plus gourmandes en haut. « Système » est protégé.", 12, T.mut);
    if (confirm && selected >= 0) { p.fill_rrect({win.x, win.y, win.w, win.h}, 22, 0x66000000); Rect d{win.x + win.w / 2 - 230, win.y + win.h / 2 - 110, 460, 220}; panel(u, d, 24, true);
      p.fill_rrect({d.x + 24, d.y + 24, 44, 44}, 14, Paint::linear(145, T.err, mix(T.err, 0xFF000000, .3f))); p.icon("triangle-alert", {d.x + 34, d.y + 34, 24, 24}, T.on_accent, 2.4f);
      u.font.draw(p, d.x + 84, d.y + 44, "Terminer « " + groups[selected].name + " » ?", 17, T.fg, Weight::SemiBold); u.font.draw(p, d.x + 84, d.y + 66, "Le travail non enregistré sera perdu.", 13, T.mut);
      u.font.draw(p, d.x + 24, d.y + 110, std::to_string(groups[selected].processes) + " processus seront arrêtés et " + bytes_kib(groups[selected].rss_kib) + " de mémoire libérés.", 13, T.fg);
      button(u, {d.x + d.w - 24 - 112 - 12 - 100, d.y + d.h - 30 - 38, 100, 38}, "Annuler", Kind::Secondary); button(u, {d.x + d.w - 24 - 112, d.y + d.h - 30 - 38, 112, 38}, "Terminer", Kind::Danger); }
  } else {
    float w = (cw - 16) / 2, ph = (b.h - 64 - 20 - 76 - 32) / 2.f; struct G { const char* t; const char* ic; int k; float mx; uint32_t c; std::string big; } gs[4] = {{"Processeur", "cpu", 0, 100, T.b1, fmt1(snap.cpu_total_pct) + " %"}, {"Mémoire", "memory-stick", 1, 100, T.b2, bytes_kib(snap.mem_used_kib)}, {"Réseau", "wifi", 2, 0, T.ok, rate(snap.net_rx_bps + snap.net_tx_bps)}, {"Disque", "hard-drive", 3, 0, T.b4, rate(snap.disk_read_bps + snap.disk_write_bps)}};
    for (int i = 0; i < 4; ++i) { Rect r{cx + (i % 2) * (w + 16), cy + (i / 2) * (ph + 16), w, ph}; panel(u, r, 22); p.icon(gs[i].ic, {r.x + 18, r.y + 16, 18, 18}, T.mut, 2.1f); u.font.draw(p, r.x + 42, r.y + 30, gs[i].t, 13, T.mut, Weight::SemiBold);
      u.font.draw(p, r.x + r.w - 20 - font.measure(gs[i].big, 20, Weight::SemiBold), r.y + 32, gs[i].big, 20, T.fg, Weight::SemiBold);
      Rect g{r.x + 20, r.y + 52, r.w - 40, r.h - 76}; for (int l = 0; l <= 4; ++l) p.fill_rrect({g.x, g.y + g.h * l / 4.f, g.w, 1}, 0, T.hair); sparkline(u, g, sv.history(uint32_t(gs[i].k)), gs[i].mx, gs[i].c);
      u.font.draw(p, g.x, r.y + r.h - 8, "il y a 30 s", 11, T.mut); u.font.draw(p, g.x + g.w - font.measure("maintenant", 11), r.y + r.h - 8, "maintenant", 11, T.mut); }
    Rect ft{cx, cy + 2 * (ph + 16) + 0, cw, 76}; panel(u, ft, 22); const char* lb[4] = {"Température", "Batterie", "Échange (swap)", "Cœurs"}; std::string vv[4] = {snap.temp_c >= 0 ? fmt1(snap.temp_c) + " °C" : "—", snap.battery_pct >= 0 ? fmt1(snap.battery_pct) + " %" : "secteur", bytes_kib(snap.swap_used_kib), std::to_string(snap.cores)};
    for (int i = 0; i < 4; ++i) { float fx = ft.x + 24 + i * (ft.w - 48) / 4; u.font.draw(p, fx, ft.y + 30, lb[i], 12, T.mut); u.font.draw(p, fx, ft.y + 56, vv[i], 18, T.fg, Weight::SemiBold); }
  }
  write_ppm(out, px, W, H);
}
int main() {
  auto blob = read_file("../../common/inter.bgf"); Font font; if (!font.load(blob.data(), blob.size())) { std::printf("police introuvable\n"); return 2; }
  BauraSvc::Services sv; for (int i = 1; i <= 60; ++i) sv.tick(uint64_t(i) * 1000);
  auto g = sv.groups(); CHECK(g.size() > 8);
  // même ordre que dans l'interface : le plus d'impact d'abord
  for (size_t i = 1; i < g.size(); ++i) CHECK(g[i - 1].impact >= g[i].impact);
  CHECK(bytes_kib(1536) == "1 Mo" || bytes_kib(1536) == "1 Mo"); CHECK(bytes_kib(2 * 1048576) == "2,0 Go"); CHECK(fmt1(3.25f) == "3,3" || fmt1(3.25f) == "3,2"); CHECK(rate(2500000) == "2,5 Mo/s");
  render(sv, font, false, Page::Overview, -1, false, "moniteur_vue-ensemble_clair.ppm"); render(sv, font, true, Page::Overview, -1, false, "moniteur_vue-ensemble_sombre.ppm");
  render(sv, font, true, Page::Apps, 0, false, "moniteur_applications_sombre.ppm"); render(sv, font, false, Page::Apps, 1, false, "moniteur_applications_clair.ppm");
  render(sv, font, true, Page::Apps, 2, true, "moniteur_terminer_sombre.ppm"); render(sv, font, false, Page::Performance, -1, false, "moniteur_performances_clair.ppm"); render(sv, font, true, Page::Performance, -1, false, "moniteur_performances_sombre.ppm");
  // action réelle : terminer le groupe sélectionné, le backend le retire de la liste
  std::string victim; for (auto& x : g) if (x.name != "Système") { victim = x.name; break; } CHECK(sv.terminate(victim) == 0); sv.tick(61000); bool still = false; for (auto& x : sv.groups()) if (x.name == victim) still = true; CHECK(!still);
  std::printf("%d échec(s)\n", fails); return fails ? 1 : 0;
}
