// BAURA OS — baura_text.h : texte réel (Inter, anti-crénelé, crénage) à partir d'un atlas BGF1.
// Ne dépend d'aucun analyseur de police : l'atlas est produit à la compilation (02-TEXTE/tools/bake_font.py).
#ifndef BAURA_TEXT_H
#define BAURA_TEXT_H
#include "baura_vector.h"
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace BauraVec {
enum class Weight : int { Regular = 0, SemiBold = 1 };

class Font {
 public:
  // `data` doit rester valide (fichier mappé depuis l'initrd, ou tableau embarqué).
  bool load(const uint8_t* data, size_t len) {
    if (len < 16 || std::memcmp(data, "BGF1", 4)) return false;
    base = data; faces = rd16(4); nsizes = rd16(6); nglyph = rd16(8); blob = rd32(12); cps = data + 16;
    sets.clear(); const uint8_t* h = cps + size_t(nglyph) * 4;
    for (int i = 0; i < faces * nsizes; ++i, h += 16) {
      Set s; s.px = h[0]; s.asc = int8_t(h[1]); s.desc = h[2]; s.line = h[3]; s.glyphs = base + rd32o(h, 4); s.kern = base + rd32o(h, 8); s.nkern = rd16o(h, 12); sets.push_back(s);
    }
    return nglyph > 0 && !sets.empty();
  }
  struct Metrics { int ascent, descent, line_height; };
  // Choisit la taille cuite la plus proche de `px` (pas de mise à l'échelle : le texte reste net)
  int pick(int px, Weight w) const { int best = 0, bd = 1 << 30; for (int i = 0; i < nsizes; ++i) { int d = std::abs(sets[size_t(int(w)) * nsizes + i].px - px); if (d < bd) { bd = d; best = i; } } return int(w) * nsizes + best; }
  Metrics metrics(int px, Weight w = Weight::Regular) const { const Set& s = sets[size_t(pick(px, w))]; return {s.asc, s.desc, s.line}; }
  float measure(const std::string& utf8, int px, Weight w = Weight::Regular) const {
    const Set& s = sets[size_t(pick(px, w))]; float x = 0; int prev = -1; size_t i = 0;
    while (i < utf8.size()) { int g = glyph_index(next_cp(utf8, i)); if (g < 0) continue; if (prev >= 0) x += kern(s, prev, g) / 64.f; x += adv(s, g) / 64.f; prev = g; }
    return x;
  }
  // Dessine `utf8` avec la ligne de base à (x, baseline). Retourne la largeur dessinée.
  float draw(Painter& p, float x, float baseline, const std::string& utf8, int px, uint32_t color, Weight w = Weight::Regular) const {
    const Set& s = sets[size_t(pick(px, w))]; float pen = x; int prev = -1; size_t i = 0;
    while (i < utf8.size()) {
      int g = glyph_index(next_cp(utf8, i)); if (g < 0) continue; if (prev >= 0) pen += kern(s, prev, g) / 64.f;
      const uint8_t* r = s.glyphs + size_t(g) * 10;
      int8_t bx = int8_t(r[2]), by = int8_t(r[3]); int w_ = r[4], h_ = r[5]; uint32_t off = rd32o(r, 6);
      const uint8_t* bm = base + blob + off; int gx = int(std::floor(pen + .5f)) + bx, gy = int(std::floor(baseline + .5f)) + by;
      for (int yy = 0; yy < h_; ++yy) for (int xx = 0; xx < w_; ++xx) { uint8_t a = bm[yy * w_ + xx]; if (a) p.put(gx + xx, gy + yy, color, a / 255.f); }
      pen += adv(s, g) / 64.f; prev = g;
    }
    return pen - x;
  }
  // Tronque avec « … » pour tenir dans max_w
  std::string ellipsize(const std::string& t, float max_w, int px, Weight w = Weight::Regular) const {
    if (measure(t, px, w) <= max_w) return t;
    std::string o = t;
    while (!o.empty()) { do o.pop_back(); while (!o.empty() && (uint8_t(o.back()) & 0xC0) == 0x80); if (measure(o + "\xE2\x80\xA6", px, w) <= max_w) return o + "\xE2\x80\xA6"; }
    return "\xE2\x80\xA6";
  }
 private:
  struct Set { int px, asc, desc, line; const uint8_t *glyphs, *kern; int nkern; };
  const uint8_t* base = nullptr; const uint8_t* cps = nullptr; int faces = 0, nsizes = 0, nglyph = 0; uint32_t blob = 0; std::vector<Set> sets;
  uint16_t rd16(size_t o) const { return uint16_t(base[o] | base[o + 1] << 8); }
  uint32_t rd32(size_t o) const { return rd32o(base, o); }
  static uint16_t rd16o(const uint8_t* p, size_t o) { return uint16_t(p[o] | p[o + 1] << 8); }
  static uint32_t rd32o(const uint8_t* p, size_t o) { return uint32_t(p[o]) | uint32_t(p[o + 1]) << 8 | uint32_t(p[o + 2]) << 16 | uint32_t(p[o + 3]) << 24; }
  static uint32_t next_cp(const std::string& s, size_t& i) {
    uint8_t c = uint8_t(s[i++]);
    if (c < 0x80) return c;
    int n = c >= 0xF0 ? 3 : c >= 0xE0 ? 2
                                      : 1;
    uint32_t cp = c & (0x3F >> n);
    while (n-- && i < s.size())
      cp = cp << 6 | (uint8_t(s[i++]) & 0x3F);
    return cp;
  }
  int glyph_index(uint32_t cp) const {  // recherche dichotomique ; repli sur « ? »
    for (int pass = 0; pass < 2; ++pass) { int lo = 0, hi = nglyph - 1; uint32_t t = pass ? '?' : cp;
      while (lo <= hi) { int m = (lo + hi) / 2; uint32_t v = rd32o(cps, size_t(m) * 4); if (v == t) return m; (v < t ? lo : hi) = v < t ? m + 1 : m - 1; } }
    return -1;
  }
  static int adv(const Set& s, int g) { return rd16o(s.glyphs + size_t(g) * 10, 0); }
  static int kern(const Set& s, int a, int b) {  // table triée par (a,b) : dichotomie
    int lo = 0, hi = s.nkern - 1; uint32_t key = uint32_t(a) << 16 | uint32_t(b);
    while (lo <= hi) { int m = (lo + hi) / 2; const uint8_t* e = s.kern + size_t(m) * 6; uint32_t k = uint32_t(rd16o(e, 0)) << 16 | rd16o(e, 2); if (k == key) return int16_t(rd16o(e, 4)); (k < key ? lo : hi) = k < key ? m + 1 : m - 1; }
    return 0;
  }
};
}  // namespace BauraVec
#endif
