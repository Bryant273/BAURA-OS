// BAURA OS — baura_a11y.h : accessibilité. (1) contraste WCAG ; (2) thèmes contraste élevé ; (3) arbre d'accessibilité + gestionnaire de focus clavier.
// Sans allocation dynamique dans le gestionnaire de focus (tableaux bornés) : utilisable côté noyau/serveur d'affichage.
#ifndef BAURA_A11Y_H
#define BAURA_A11Y_H
#include "baura_ui.h"
#include <cmath>
#include <cstdio>
#include <string>
namespace BauraA11y {
using namespace BauraVec;
// ---------- 1. Contraste (WCAG 2.2, luminance relative sRGB) ----------
inline float lin(float c) { return c <= .04045f ? c / 12.92f : std::pow((c + .055f) / 1.055f, 2.4f); }
inline float luminance(uint32_t c) { return .2126f * lin(((c >> 16) & 255) / 255.f) + .7152f * lin(((c >> 8) & 255) / 255.f) + .0722f * lin((c & 255) / 255.f); }
inline float contrast(uint32_t a, uint32_t b) { float la = luminance(a), lb = luminance(b); if (la < lb) std::swap(la, lb); return (la + .05f) / (lb + .05f); }
inline uint32_t over(uint32_t fg, uint32_t bg) {   // composite fg (avec alpha) sur bg opaque
  float a = ((fg >> 24) & 255) / 255.f; auto ch = [&](int s) { return uint32_t(((fg >> s) & 255) * a + ((bg >> s) & 255) * (1 - a) + .5f); };
  return 0xFF000000 | ch(16) << 16 | ch(8) << 8 | ch(0);
}
// Plus mauvais contraste d'une couleur sur un fond translucide posé sur deux fonds extrêmes (fond d'écran clair / foncé)
inline float worst(uint32_t fg, uint32_t surface, uint32_t w0, uint32_t w1) { float a = contrast(over(fg, over(surface, w0)), over(surface, w0)), b = contrast(over(fg, over(surface, w1)), over(surface, w1)); return a < b ? a : b; }
// Ajuste une couleur (assombrit en clair, éclaircit en sombre) jusqu'au ratio demandé sur le pire fond.
inline uint32_t fix(uint32_t fg, uint32_t surface, uint32_t w0, uint32_t w1, float ratio, bool darken) {
  for (int i = 0; i < 64 && worst(fg, surface, w0, w1) < ratio; ++i) { auto st = [&](int s) { int v = int((fg >> s) & 255); v = darken ? v * 94 / 100 : v + (255 - v) * 8 / 100; return uint32_t(v); }; fg = 0xFF000000 | st(16) << 16 | st(8) << 8 | st(0); }
  return fg;
}
// ---------- 2. Thèmes à contraste élevé : aucune transparence, bords pleins de 2 px, focus jaune ----------
inline constexpr BauraBrand::Tokens HC_DARK{0xFF8FA8FF, 0xFFC8B6FF, 0xFFFFFFFF, 0xFFFFD84D, 0xFFFFFFFF, 0xFFE6E6E6,
  0xFF000000, 0xFF000000, 0xFFFFFFFF, 0xFF6B6B6B, 0xFF2A2A5A, 0xFF3A3A7A, 0xFF101010, 0xFFFFFFFF,
  0xFF000000, 0xFF101010, 0xFFFFFFFF, 0xFFFFD84D, 0xFFFF8A8A, 0xFF7CFFB0, 0xFF000000, 0xFF000000, 0.f, 1.f, 0.f, 0xFF000000};
inline constexpr BauraBrand::Tokens HC_LIGHT{0xFF1A12B8, 0xFF4B2AA8, 0xFF2A1A80, 0xFF6B4F00, 0xFF000000, 0xFF1F1F1F,
  0xFFFFFFFF, 0xFFFFFFFF, 0xFF000000, 0xFF8A8A8A, 0xFFDCDCFF, 0xFFC8C8FF, 0xFFFFFFFF, 0xFF000000,
  0xFFFFFFFF, 0xFFF2F2F2, 0xFF000000, 0xFF1A12B8, 0xFF9A0010, 0xFF005A2B, 0xFF000000, 0xFF000000, 0.f, 1.f, 0.f};

// ---------- 3. Arbre d'accessibilité ----------
enum class Role : uint8_t { Window, Group, Button, CheckBox, Radio, RadioGroup, Switch, TextField, List, ListItem, Tab, TabList, Slider, Menu, MenuItem, Label, Image, Dialog, ProgressBar };
inline const char* role_name(Role r) { static const char* n[] = {"fenêtre", "groupe", "bouton", "case à cocher", "radio", "groupe de radios", "interrupteur", "champ de texte", "liste", "élément de liste", "onglet", "liste d'onglets", "curseur", "menu", "élément de menu", "libellé", "image", "boîte de dialogue", "barre de progression"}; return n[int(r)]; }
struct Node {
  Role role = Role::Group; std::string name, description; Rect rect{};
  bool focusable = false, disabled = false, checked = false, expanded = false, selected = false, hidden = false; float value = -1;   // value : curseur / progression, 0..1
  std::vector<Node> kids;
};
// Parcours dans l'ordre de lecture (= ordre de tabulation) ; en RTL, l'ordre visuel est miroir mais l'ordre LOGIQUE reste identique.
inline void collect(const Node& n, std::vector<const Node*>& out) { if (n.hidden) return; if (n.focusable && !n.disabled) out.push_back(&n); for (auto& k : n.kids) collect(k, out); }
struct FocusManager {
  std::vector<const Node*> order; int cur = -1; const Node* root = nullptr;
  void rebuild(const Node& r) { const Node* old = cur >= 0 && cur < int(order.size()) ? order[cur] : nullptr; root = &r; order.clear(); collect(r, order); cur = -1; if (old) for (size_t i = 0; i < order.size(); ++i) if (order[i] == old) cur = int(i); }
  const Node* tab(bool shift) { if (order.empty()) return nullptr; cur = cur < 0 ? (shift ? int(order.size()) - 1 : 0) : (cur + (shift ? int(order.size()) - 1 : 1)) % int(order.size()); return order[size_t(cur)]; }
  // Flèches dans un groupe de radios / une liste d'onglets : reste dans le groupe, boucle ; en RTL la flèche droite recule.
  const Node* arrow(int dx, bool rtl, const Node& group) {
    if (cur < 0) return nullptr;
    std::vector<const Node*> g;
    collect(group, g);
    int me = -1;
    for (size_t i = 0; i < g.size(); ++i)
      if (g[i] == order[size_t(cur)]) me = int(i);
    if (me < 0) return nullptr;
    int step = rtl ? -dx : dx; int nx = (me + step + int(g.size())) % int(g.size()); for (size_t i = 0; i < order.size(); ++i) if (order[i] == g[size_t(nx)]) cur = int(i); return order[size_t(cur)]; }
  const Node* focused() const { return cur >= 0 ? order[size_t(cur)] : nullptr; }
};
// Annonce vocale (lecteur d'écran) : « nom, rôle, état »
inline std::string announce(const Node& n) { std::string s = n.name + ", " + role_name(n.role); if (n.role == Role::CheckBox || n.role == Role::Switch || n.role == Role::Radio) s += n.checked ? ", activé" : ", désactivé"; if (n.selected) s += ", sélectionné"; if (n.disabled) s += ", indisponible"; if (n.value >= 0) s += ", " + std::to_string(int(n.value * 100)) + " pour cent"; if (!n.description.empty()) s += ". " + n.description; return s; }
inline void dump(const Node& n, std::string& out, int d = 0) { out.append(size_t(d) * 2, ' '); out += std::string("- ") + role_name(n.role) + " « " + n.name + " »"; if (n.focusable) out += n.disabled ? " [indisponible]" : " [focus]"; out += "\n"; for (auto& k : n.kids) dump(k, out, d + 1); }
}  // namespace BauraA11y
#endif
