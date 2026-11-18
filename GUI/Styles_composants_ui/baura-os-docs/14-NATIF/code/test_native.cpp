// « Avant / après » : la fenêtre Fichiers du bureau actuel (verre, grandes cartes) face à la version NATIVE (matières pleines, menus, détails, barre d'état).
#include <cstdio>
#include "baura_native.h"
#include "baura_apps.h"
#include "bmp_out.h"
using namespace BauraNative;
static int fails = 0;
#define CHECK(c) do { if (!(c)) { std::printf("ECHEC: %s (ligne %d)\n", #c, __LINE__); ++fails; } } while (0)
static void native_files(Ui& u, Rect w, const Density& D, bool dark, bool with_menu, int hotrow) {
  Surface S = surface(u.T, dark); Rect b = window(u, w, D, S, "Documents — Fichiers", "folder");
  menubar(u, {b.x, b.y, b.w, D.menubar}, S, {{"Fichier", 0}, {"Édition", 0}, {"Affichage", 0}, {"Aller", 0}, {"Outils", 0}, {"Aide", 0}}, with_menu ? 1 : -1, true);
  Rect tb{b.x, b.y + D.menubar, b.w, D.toolbar}; u.p.fill_rrect(tb, 0, S.content); u.p.fill_rrect({tb.x, tb.y + tb.h - 1, tb.w, 1}, 0, S.line);
  float cy = tb.y + (tb.h - 28) / 2; tool_button(u, {tb.x + 10, cy, 30, 28}, "arrow-left"); tool_button(u, {tb.x + 42, cy, 30, 28}, "arrow-right", false); tool_button(u, {tb.x + 74, cy, 30, 28}, "arrow-up"); tool_button(u, {tb.x + 106, cy, 30, 28}, "rotate-cw");
  Rect path{tb.x + 146, cy - 1, tb.w - 146 - 250, 30}; field(u, path, S, D); u.font.draw(u.p, path.x + 12, path.y + 20, "Accueil  /  Documents", 13, u.T.fg);
  Rect sr{tb.x + tb.w - 236, cy - 1, 170, 30}; field(u, sr, S, D); u.p.icon("search", {sr.x + 8, sr.y + 7, 16, 16}, u.T.mut, 2); u.font.draw(u.p, sr.x + 30, sr.y + 20, "Rechercher", 13, u.T.mut);
  tool_button(u, {tb.x + tb.w - 54, cy, 30, 28}, "list", true, false, true);
  float top = tb.y + tb.h, bot = b.y + b.h - D.status; Rect side{b.x, top, 190, bot - top};
  sidebar(u, side, S, D, {{"", "FAVORIS", true, false}, {"@folder-home", "Dossier personnel", false, false}, {"@folder-desktop", "Bureau", false, false}, {"@folder-documents", "Documents", false, true}, {"@folder-pictures", "Images", false, false}, {"@folder-music", "Musique", false, false}, {"@folder-downloads", "Téléchargements", false, false}, {"", "APPAREILS", true, false}, {"@disks", "Disque principal", false, false}, {"@trash", "Corbeille", false, false}});
  std::vector<Row> rows = {{"folder", {"Projets", "Hier, 18:42", "Dossier", "—"}}, {"folder", {"Factures 2026", "12 oct., 09:10", "Dossier", "—"}}, {"file-pdf", {"Contrat de location.pdf", "10 oct., 16:02", "PDF", "1,4 Mo"}}, {"file-doc", {"Lettre de motivation.docx", "8 oct., 11:27", "Document", "48 Ko"}},
    {"file-sheet", {"Budget.xlsx", "6 oct., 20:15", "Feuille de calcul", "212 Ko"}}, {"file-slides", {"Présentation BAURA.pptx", "3 oct., 14:50", "Présentation", "3,8 Mo"}}, {"file-image", {"Photo de famille.png", "1 oct., 08:33", "Image", "2,2 Mo"}}, {"file-audio", {"Mémo vocal.mp3", "28 sept., 19:01", "Audio", "960 Ko"}}, {"file-archive", {"Sauvegarde.zip", "20 sept., 22:40", "Archive", "126 Mo"}}, {"file-code", {"main.cpp", "18 sept., 10:12", "Code source", "12 Ko"}}, {"file-text", {"Notes.txt", "15 sept., 07:55", "Texte", "3 Ko"}}, {"file-unknown", {"données.bin", "2 sept., 13:20", "Fichier", "88 Ko"}}};
  details(u, {side.x + side.w, top, b.w - side.w, bot - top}, S, D, {{"Nom", 0, false}, {"Modifié", 100, false}, {"Type", 100, false}, {"Taille", 100, true}}, rows, 0, true, 3, 3, hotrow);
  statusbar(u, {b.x, bot, b.w, D.status}, S, "12 éléments · 1 sélectionné (48 Ko)", "186 Go libres"); grip(u, w);
}
int main() {
  auto blob = read_file("../../common/inter.bgf"); Font font; if (!font.load(blob.data(), blob.size())) return 2;
  CHECK(BauraNative::COMPACT.row < BauraNative::COMFORTABLE.row && BauraNative::COMPACT.titlebar < 46);
  const int W = 1280, H = 720;
  for (int dark = 0; dark < 2; ++dark) {
    std::vector<uint32_t> px(size_t(W) * H); BGEFramebuffer fb{px.data(), W, H, W, px.size()}; BGERenderer rd(fb); Painter p(rd); const auto& T = dark ? BauraBrand::DARK : BauraBrand::LIGHT; Ui u{p, font, T};
    p.fill_rrect({0, 0, float(W), float(H)}, 0, dark ? Paint::linear(150, 0xFF241C66, 0xFF0A0820) : Paint::linear(150, 0xFFF7F1FF, 0xFFCFC6FF));
    Surface S = surface(T, dark); native_files(u, {60, 50, 920, 560}, BauraNative::COMPACT, dark, true, 5);
    // menu « Édition » ouvert + menu contextuel
    Rect mr = context_menu(u, {128, 112}, S, {{"Annuler", "Ctrl+Z", false, true, false}, {"Rétablir", "Ctrl+Maj+Z", false, false, false}, {"", "", true, false, false}, {"Couper", "Ctrl+X", false, true, true}, {"Copier", "Ctrl+C", false, true, false}, {"Coller", "Ctrl+V", false, true, false}, {"", "", true, false, false}, {"Sélectionner tout", "Ctrl+A", false, true, false}}); (void)mr;
    context_menu(u, {640, 330}, S, {{"Ouvrir", "Entrée", false, true, true}, {"Ouvrir avec…", "", false, true, false}, {"", "", true, false, false}, {"Renommer", "F2", false, true, false}, {"Copier", "Ctrl+C", false, true, false}, {"Mettre à la corbeille", "Suppr", false, true, false}, {"", "", true, false, false}, {"Propriétés", "Alt+Entrée", false, true, false}});
    write_ppm(dark ? "natif_fichiers_sombre.ppm" : "natif_fichiers_clair.ppm", px, W, H);
  }
  // avant / après côte à côte (clair)
  { const int W2 = 1280, H2 = 1160; std::vector<uint32_t> px(size_t(W2) * H2); BGEFramebuffer fb{px.data(), W2, H2, W2, px.size()}; BGERenderer rd(fb); Painter p(rd); const auto& T = BauraBrand::LIGHT; Ui u{p, font, T};
    p.fill_rrect({0, 0, float(W2), float(H2)}, 0, Paint::linear(150, 0xFFF7F1FF, 0xFFCFC6FF));
    font.draw(p, 40, 36, "AVANT — verre partout, cartes arrondies, pas de barre de menus, contrôles surdimensionnés (impression « appli web »)", 15, T.fg, Weight::SemiBold);
    Rect w1{40, 56, 880, 500}; Rect b1 = window_frame(u, w1, "Fichiers — Documents", "folder", 0xFF6D66F2, 0xFF4F46E5, true); BauraApps::Ctx cx{u, font}; p.set_clip(int(b1.x), int(b1.y), int(b1.w), int(b1.h)); BauraApps::files(cx, b1); p.reset_clip();
    font.draw(p, 40, 606, "APRÈS — mode natif : matières pleines, menus avec mnémoniques, barre d'outils compacte, liste en détails, barre d'état, poignée", 15, T.fg, Weight::SemiBold);
    native_files(u, {40, 626, 880, 500}, BauraNative::COMPACT, false, false, -1);
    // légende
    const char* leg[] = {"Châssis de fenêtre 34 px (au lieu de 46)", "Rayon 10 (au lieu de 22)", "Verre réservé au châssis ; contenu PLEIN", "Barre de menus + mnémoniques (Alt)", "Boutons d'outils sans cadre, 30×28", "Liste en détails : tri, zébrage, focus clavier", "Barre d'état + poignée de redimensionnement", "Menus contextuels avec raccourcis"};
    for (int i = 0; i < 8; ++i) { p.fill_rrect({950, 660.f + i * 52, 6, 28}, 3, T.b1); font.draw(p, 968, 680.f + i * 52, leg[i], 13, T.fg); }
    write_ppm("natif_avant-apres.ppm", px, W2, H2); }
  std::printf("%d échec(s)\n", fails); return fails ? 1 : 0;
}
