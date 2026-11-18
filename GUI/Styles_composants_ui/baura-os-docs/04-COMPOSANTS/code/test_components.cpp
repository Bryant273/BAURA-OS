// Planche de tous les composants (états inclus), en clair et en sombre.
#include "baura_ui.h"
#include "bmp_out.h"
using namespace BauraUI;
static void sheet(const Tokens& T, bool dark, const char* out, const Font& font) {
  const int W = 1180, H = 860; std::vector<uint32_t> px(size_t(W) * H); BGEFramebuffer fb{px.data(), W, H, W, px.size()}; BGERenderer r(fb); Painter p(r); Ui u{p, font, T};
  p.fill_rrect({0, 0, W, H}, 0, dark ? Paint::linear(150, 0xFF1A1450, 0xFF3B1F8F) : Paint::linear(150, 0xFFF1E4FF, 0xFFA99BFF));
  p.symbol({W - 190.f, -40, 150, 320}, T.b1, T.b2, T.b3, T.b4);
  auto H1 = [&](float x, float y, const char* s) { font.draw(p, x, y, s, 12, T.mut, Weight::SemiBold); };
  float x = 30, y = 40; H1(x, y, "BOUTONS · normal / survol / appui / focus / désactivé"); y += 14;
  Kind ks[] = {Kind::Primary, Kind::Secondary, Kind::Ghost, Kind::Danger}; State ss[] = {State::Normal, State::Hover, State::Pressed, State::Focus, State::Disabled};
  const char* kn[] = {"Continuer", "Annuler", "Ignorer", "Supprimer"};
  for (int k = 0; k < 4; ++k) for (int s = 0; s < 5; ++s) button(u, {x + s * 150.f, y + k * 48.f, 132, 36}, kn[k], ks[k], ss[s], k == 0 && s == 0 ? "check" : nullptr);
  y += 4 * 48 + 14; H1(x, y, "CHAMPS · normal / survol / focus / erreur / mot de passe"); y += 14;
  input(u, {x, y, 220, 40}, "", "Rechercher…", State::Normal, "search"); input(u, {x + 240, y, 220, 40}, "ahmed", "", State::Hover, "user");
  input(u, {x + 480, y, 220, 40}, "Paramètres", "", State::Focus, "search", false, 5); input(u, {x, y + 56, 220, 40}, "mot@", "", State::Error, "mail"); input(u, {x + 240, y + 56, 220, 40}, "secret42", "", State::Focus, "lock", true, 8);
  input(u, {x + 480, y + 56, 220, 40}, "", "Désactivé", State::Disabled);
  y += 124; H1(x, y, "CONTRÔLES"); y += 14;
  checkbox(u, {x, y, 20, 20}, false); checkbox(u, {x + 34, y, 20, 20}, true); checkbox(u, {x + 68, y, 20, 20}, false, State::Focus); radio(u, {x + 110, y, 20, 20}, false); radio(u, {x + 144, y, 20, 20}, true);
  toggle(u, {x + 190, y - 3, 44, 26}, false); toggle(u, {x + 246, y - 3, 44, 26}, true); slider(u, {x + 320, y - 2, 200, 24}, .62f); progress(u, {x + 550, y + 6, 160, 8}, .7f); spinner(u, {x + 760, y + 10}, 11, .1f);
  badge(u, {x + 800, y, 26, 20}, "3", T.err); chip(u, {x + 840, y - 3, 90, 26}, "Récents", true); chip(u, {x + 940, y - 3, 90, 26}, "Photos", false);
  y += 40; H1(x, y, "ONGLETS DE FENÊTRE · CONTRÔLE SEGMENTÉ"); y += 12;
  Rect tb{x, y, 560, 40}; p.fill_rrect(tb, 14, T.win_bar); tabs(u, tb, {{"Documents", "folder", true}, {"Images", "image", false}}); segmented(u, {x + 590, y + 3, 320, 34}, {{"Jour", false}, {"Semaine", true}, {"Mois", false}});
  y += 64; H1(x, y, "LISTE · MENU CONTEXTUEL · INFO-BULLE · NOTIFICATION"); y += 12;
  panel(u, {x, y, 330, 190}, 20); list_row(u, {x + 8, y + 8, 314, 52}, "folder", "Documents", "12 éléments", State::Normal, "hier", true); list_row(u, {x + 8, y + 64, 314, 52}, "image", "Images", "48 éléments", State::Hover, "lun."); list_row(u, {x + 8, y + 120, 314, 52}, "music", "Musique", "230 éléments", State::Normal, "sam.");
  menu(u, {x + 360, y}, 230, {{"Ouvrir", "folder-open", "Entrée"}, {"Copier", "copy", "Ctrl+C"}, {"Renommer", "pencil", "F2", false, false, false}, {"", nullptr, "", true}, {"Afficher les icônes", "layout-grid", "", false, false, true, true}, {"Supprimer", "trash-2", "Suppr", false, false, false, false, true}}, 1);
  tooltip(u, {x + 620, y + 10}, "Paramètres"); toast(u, {x + 620, y + 60, 332, 66}, "bell", 0xFFFB7185, 0xFFE11D48, "Mise à jour disponible", "BAURA OS 0.2 est prêt à installer.");
  app_tile(u, {x + 620, y + 140, 44, 44}, "folder", 0xFF6D66F2, 0xFF4F46E5); app_tile(u, {x + 674, y + 140, 44, 44}, "settings", 0xFF9AA0C8, 0xFF6B7199); app_tile(u, {x + 728, y + 140, 44, 44}, "terminal", 0xFF34D399, 0xFF047857); app_tile(u, {x + 782, y + 140, 44, 44}, "shopping-bag", 0xFFFB923C, 0xFFEA580C);
  write_ppm(out, px, W, H);
}
int main() { auto blob = read_file("../../common/inter.bgf"); Font f; f.load(blob.data(), blob.size()); sheet(BauraBrand::LIGHT, false, "comp_light.ppm", f); sheet(BauraBrand::DARK, true, "comp_dark.ppm", f); }
