// BAURA OS — baura_appicons.h : rendu des icônes d'APPLICATIONS (grille 64×64, tuile dégradée + motifs + ombre + point doré).
// Données : appicons_data.h (généré depuis 11-ICONES-APPLICATIONS/tools). Dessins originaux, aucun logo de marque.
#ifndef BAURA_APPICONS_H
#define BAURA_APPICONS_H
#include "baura_vector.h"
#include "appicons_data.h"
#include <unordered_map>
namespace BauraVec::appicons {
// Résolution : id exact → alias (ids freedesktop/Papirus) → application connue → repli « application générique »
inline const Icon* find(const char* id) {
  if (!id) return nullptr;
  for (size_t i = 0; i < COUNT; ++i) if (!std::strcmp(ALL[i].id, id)) return &ALL[i];
  for (size_t i = 0; i < ALIAS_COUNT; ++i) if (!std::strcmp(ALIASES[i].from, id)) return &ALL[ALIASES[i].idx];
  for (size_t i = 0; i < KNOWN_COUNT; ++i) if (!std::strcmp(KNOWN_APPS[i].from, id)) return &ALL[KNOWN_APPS[i].idx];
  return nullptr;
}
inline const Icon* resolve(const char* id) { const Icon* r = find(id); if (!r) r = find("app-generic"); return r; }
struct Style { bool shadow = true; bool tile_gloss = true; float opacity = 1.f; };
inline const Path& cached(const Elem& e) { static std::unordered_map<const Elem*, Path> c; auto it = c.find(&e); if (it == c.end()) it = c.emplace(&e, Path::fromSVG(e.d, 20)).first; return it->second; }
// Dessine l'icône dans r (carré conservé, centré). Retourne false si l'id est inconnu (le repli est alors dessiné).
inline bool draw(Painter& p, const char* id, Rect r, const Style& st = {}) {
  const Icon* ic = find(id); bool ok = ic != nullptr; if (!ic) ic = resolve("app-generic");
  float size = std::min(r.w, r.h), sc = size / 64.f; Vec2 pos{r.x + (r.w - size) / 2, r.y + (r.h - size) / 2};
  float keep = p.opacity(); if (st.opacity != 1.f) p.set_opacity(keep * st.opacity);
  if (ic->tile) {
    Rect t{pos.x + 4 * sc, pos.y + 4 * sc, 56 * sc, 56 * sc};
    if (st.shadow && size >= 28) for (int k = 3; k >= 1; --k) p.fill_rrect({t.x - k * sc * .8f, t.y + sc * (1.6f + k * .5f), t.w + 2 * k * sc * .8f, t.h}, 15 * sc + k * sc * .8f, Paint(argb(10 + (4 - k) * 3, 0, 0, 0)));
    p.fill_rrect(t, 15 * sc, Paint::linear(180, ic->bg0, ic->bg1));
    if (st.tile_gloss && size >= 24) p.stroke_rrect(t, 15 * sc, std::max(1.f, .6f * sc), Paint::linear(180, 0x55FFFFFF, 0x10FFFFFF));
  }
  for (size_t k = 0; k < ic->n; ++k) {
    const Elem& e = ic->e[k]; if (e.min_px > size) continue; const Path& path = cached(e);
    if (e.flags & 4) { p.stroke_path(path, pos, sc, e.sw, Paint(e.sc)); continue; }
    if (st.shadow && (e.flags & 1) && size >= 28) p.fill_path(path, {pos.x, pos.y + 1.6f * sc}, sc, Paint(0x2E000000));
    p.fill_path(path, pos, sc, (e.flags & 2) ? Paint::linear(e.angle, e.c0, e.c1) : Paint(e.c0));
  }
  p.set_opacity(keep); return ok;
}
}  // namespace BauraVec::appicons
#endif
