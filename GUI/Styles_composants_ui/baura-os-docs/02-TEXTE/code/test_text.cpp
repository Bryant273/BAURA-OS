// Rendu de démonstration : texte Inter anti-crénelé vs. police 5×7 actuelle du dépôt.
#include "baura_text.h"
#include "bmp_out.h"
using namespace BauraVec;
int main(int argc, char** argv) {
  const int W = 900, H = 520; std::vector<uint32_t> px(size_t(W) * H); BGEFramebuffer fb{px.data(), W, H, W, px.size()}; BGERenderer r(fb); Painter p(r);
  auto blob = read_file(argc > 1 ? argv[1] : "../common/inter.bgf"); Font font; if (!font.load(blob.data(), blob.size())) { std::puts("atlas introuvable"); return 1; }
  p.fill_rrect({0, 0, W, H}, 0, Paint::linear(150, 0xFFF1E4FF, 0xFFA99BFF));
  p.fill_rrect({20, 20, 430, 480}, 22, 0xB0FFFFFF); p.fill_rrect({470, 20, 410, 480}, 22, 0xFF101827);
  uint32_t fg = 0xFF17133D, mut = 0xFF5F5A88; float y = 78;
  font.draw(p, 44, y, "Bienvenue", 44, fg, Weight::SemiBold); y += 44;
  font.draw(p, 44, y, "Installons BAURA OS.", 28, 0xFF4F46E5, Weight::SemiBold); y += 36;
  font.draw(p, 44, y, "Titre de fenêtre · 20 px semi-gras", 20, fg, Weight::SemiBold); y += 30;
  font.draw(p, 44, y, "Corps 16 px — élégant, simple et net.", 16, fg); y += 26;
  font.draw(p, 44, y, "Texte courant 14 px : Paramètres, Téléchargements, Écran…", 14, fg); y += 22;
  font.draw(p, 44, y, "Métadonnées 12 px · 12 oct. 2026 · 3,2 Mo · 100 %", 12, mut); y += 34;
  font.draw(p, 44, y, "AVA WAVE Yo To — crénage actif", 20, fg); y += 26;
  float w0 = font.measure("Un très long nom de fichier qui dépasse la largeur disponible.txt", 14);
  font.draw(p, 44, y, font.ellipsize("Un très long nom de fichier qui dépasse la largeur disponible.txt", 220, 14), 14, fg);
  char b[64]; std::snprintf(b, sizeof b, "(largeur réelle : %.0f px, coupée à 220 px)", w0); y += 20; font.draw(p, 44, y, b, 12, mut);
  // colonne de droite : l'ancien rendu 5×7 du dépôt
  r.draw_text(494, 44, "AVANT : POLICE 5X7 DU DEPOT", 0xFFB7C2D0, 1);
  r.draw_text(494, 80, "Bienvenue", 0xFFF4F7FB, 4); r.draw_text(494, 130, "Installons BAURA OS", 0xFF6BD5E1, 2);
  r.draw_text(494, 170, "Corps de texte : Parametres, Telechargements", 0xFFF4F7FB, 1);
  r.draw_text(494, 190, "Pas d'accents, pas de minuscules distinctes", 0xFFB7C2D0, 1);
  font.draw(p, 494, 260, "APRÈS : Inter cuit (BGF1)", 14, 0xFFB7C2D0, Weight::SemiBold);
  font.draw(p, 494, 300, "Bienvenue", 44, 0xFFF4F7FB, Weight::SemiBold); font.draw(p, 494, 336, "Installons BAURA OS", 20, 0xFFC4B5FD, Weight::SemiBold);
  font.draw(p, 494, 366, "Corps de texte : Paramètres, Téléchargements", 14, 0xFFF4F7FB); font.draw(p, 494, 388, "Accents, € « » … œ, vraies minuscules", 14, 0xFFB7C2D0);
  write_ppm("text.ppm", px, W, H); std::puts("text.ppm écrit");
}
