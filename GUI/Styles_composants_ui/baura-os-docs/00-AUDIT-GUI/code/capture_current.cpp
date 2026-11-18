// Capture TOUS les écrans du GUI actuel du dépôt (sans le modifier) pour l'inventaire de l'audit.
// Build : voir ../../build_all.sh (compile contre GUI/ du dépôt).
#include "BGE/renderer.h"
#include "BGE/ui_framework.h"
#include "DESKTOP/baura_desktop.h"
#include "LOGIN_SCREEN/login_screen.h"
#include "UI_FRAMEWORK/baura_hybrid_taskbar.h"
#include "UI_FRAMEWORK/system_bubble.h"
#include "UI_FRAMEWORK/ui_extensions.h"
#include "installer_gui.h"
#include "bmp_out.h"
#include <cstdio>
#include <string>
#include <vector>
static const int W = 1024, H = 640;
static int ok_stage(void*, BauraInstallStage) { return 0; }
static void save(const std::vector<uint32_t>& px, const char* name) { write_ppm((std::string(name) + ".ppm").c_str(), px, W, H); }
int main() {
  std::vector<uint32_t> px(size_t(W) * H); BGEFramebuffer fb{px.data(), W, H, W, px.size()}; BGERenderer r(fb);
  BauraTaskbarStatus st{true, true, 82, false, "FR", "12:04", 41, true};
  // 1. bureau + barre + widgets + fenêtre
  { BauraDesktop d; BauraWindow win("Fichiers", 180, 90, 560, 360); BauraButton b(24, 60, 150, 36, "Nouveau"); win.add_widget(&b); d.add_window(&win);
    d.taskbar().set_status(st); d.taskbar().set_notification_count(3); d.taskbar().set_media({"Heures indigo", "BAURA", true, true});
    d.taskbar().upsert_app(1, "Fichiers", "folder", true); d.taskbar().upsert_app(2, "Paramètres", "settings", false); d.taskbar().upsert_app(3, "HOUB", "network", false);
    d.render_frame(r, 0.f); save(px, "ui_bureau"); }
  // 2-4. panneaux
  for (int k = 0; k < 3; ++k) { BauraDesktop d; d.taskbar().set_status(st); d.taskbar().set_notification_count(3); d.taskbar().upsert_app(1, "Fichiers", "folder", true);
    if (k == 0) d.taskbar().toggle_start_menu(); if (k == 1) { d.taskbar().toggle_search(); for (char c : std::string("param")) d.taskbar().handle_search_character(c); } if (k == 2) d.taskbar().toggle_notifications();
    d.render_frame(r, 0.f); save(px, k == 0 ? "ui_demarrer" : k == 1 ? "ui_recherche" : "ui_notifications"); }
  // 5. connexion
  { BauraLoginScreen l; l.set_user_name("ahmed"); l.set_message("Mot de passe incorrect"); l.handle_character('a'); l.handle_character('b'); l.render(r); save(px, "ui_connexion"); }
  // 6-12. installateur
  { BauraInstaller in; in.set_backend(ok_stage, nullptr); const char* n[] = {"ui_inst_accueil", "ui_inst_scan", "ui_inst_partition", "ui_inst_format", "ui_inst_copie", "ui_inst_fin"};
    for (int i = 0; i < 6; ++i) { in.render(r); save(px, n[i]); in.run_stage(); } }
  // 13. bulle de progression + graphique
  { r.clear(0xFF101827u); BauraSystemBubble b; b.update_progress("Copie du système…", .62f); b.set_visible(true); b.render(r, 40, 40);
    float data[40]; for (int i = 0; i < 40; ++i) data[i] = .4f + .3f * ((i * 37) % 11) / 11.f; render_performance_graph(r, 40, 140, 300, 90, data, 40, 0xFF6BD5E1); save(px, "ui_bulle_graphique"); }
  std::puts("captures ok");
}
