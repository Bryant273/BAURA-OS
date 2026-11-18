// BAURA OS — baura_bidi.h : mise en forme du texte arabe (jonction des lettres) + réordonnancement bidirectionnel simplifié.
// Entrée : texte UTF-8 en ordre LOGIQUE ; sortie : UTF-8 en ordre VISUEL (gauche → droite), prêt pour Font::draw() qui dessine toujours de gauche à droite.
// Couvre : lettres arabes de base, ligatures lam-alef, chiffres et mots latins dans une phrase arabe, parenthèses miroir.
// NE couvre PAS : voyelles/diacritiques (supprimés), lettres persanes/ourdoues (pe, che, gaf…), tatweel, embeddings explicites (LRE/RLE), algorithme UAX #9 complet.
// Pour un usage noyau : remplacer std::vector par des tampons fixes (le texte d'interface est court : 256 caractères suffisent).
#ifndef BAURA_BIDI_H
#define BAURA_BIDI_H
#include "arabic_forms.h"
#include <cstdint>
#include <string>
#include <vector>
namespace BauraText {
using U32 = std::vector<uint32_t>;
inline U32 utf8_decode(const std::string& s) {
  U32 o; for (size_t i = 0; i < s.size();) { uint8_t c = uint8_t(s[i]); uint32_t cp; int n;
    if (c < 0x80) { cp = c; n = 1; } else if ((c >> 5) == 6) { cp = c & 31; n = 2; } else if ((c >> 4) == 14) { cp = c & 15; n = 3; } else { cp = c & 7; n = 4; }
    for (int k = 1; k < n && i + k < s.size(); ++k)
      cp = (cp << 6) | (uint8_t(s[i + k]) & 63);
    o.push_back(cp);
    i += size_t(n);
  }
  return o;
}
inline std::string utf8_encode(const U32& v) {
  std::string o; for (uint32_t c : v) { if (c < 0x80) o += char(c); else if (c < 0x800) { o += char(0xC0 | c >> 6); o += char(0x80 | (c & 63)); } else if (c < 0x10000) { o += char(0xE0 | c >> 12); o += char(0x80 | ((c >> 6) & 63)); o += char(0x80 | (c & 63)); } else { o += char(0xF0 | c >> 18); o += char(0x80 | ((c >> 12) & 63)); o += char(0x80 | ((c >> 6) & 63)); o += char(0x80 | (c & 63)); } }
  return o;
}
inline const ArForm* ar_form(uint32_t c) { for (unsigned i = 0; i < AR_FORMS_N; ++i) if (AR_FORMS[i].base == c) return &AR_FORMS[i]; return nullptr; }
inline bool is_ar_mark(uint32_t c) { return c >= 0x064B && c <= 0x0652; }

// 1. Jonction : lettres de base -> formes de présentation (isolée / finale / initiale / médiane) + ligatures lam-alef.
inline U32 shape(const U32& in) {
  U32 s; for (uint32_t c : in) if (!is_ar_mark(c)) s.push_back(c);          // diacritiques ignorés (voir limites)
  U32 out; const ArForm* prev = nullptr;
  for (size_t i = 0; i < s.size(); ++i) { uint32_t c = s[i]; const ArForm* f = ar_form(c);
    if (!f) { out.push_back(c); prev = nullptr; continue; }
    if (c == 0x0644 && i + 1 < s.size()) {                                     // lam + alef => ligature
      for (unsigned k = 0; k < AR_LIGS_N; ++k) if (AR_LIGS[k].alef == s[i + 1]) {
        bool jp = prev && prev->ini; out.push_back(jp ? AR_LIGS[k].fin : AR_LIGS[k].iso); ++i; prev = nullptr; goto next; } }
    { const ArForm* nx = i + 1 < s.size() ? ar_form(s[i + 1]) : nullptr;
      bool jp = prev && prev->ini && f->fin, jn = f->ini && nx && nx->fin;
      out.push_back(jp && jn ? f->med : jp ? f->fin : jn ? f->ini : f->iso); prev = f; }
    next:;
  }
  return out;
}

// 2. Classes bidi simplifiées
enum Cls : uint8_t { L, R, N, NEU };   // L latin, R arabe/hébreu, N chiffre, NEU neutre
inline Cls classify(uint32_t c) {
  if ((c >= '0' && c <= '9') || (c >= 0x0660 && c <= 0x0669)) return N;
  if ((c >= 0x0590 && c <= 0x08FF) || (c >= 0xFB1D && c <= 0xFDFF) || (c >= 0xFE70 && c <= 0xFEFF)) return (c == 0x060C || c == 0x061B || c == 0x061F) ? NEU : R;
  if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c >= 0xC0) return L;
  return NEU;
}
inline uint32_t mirror_char(uint32_t c) { switch (c) { case '(': return ')'; case ')': return '('; case '[': return ']'; case ']': return '['; case '{': return '}'; case '}': return '{'; case '<': return '>'; case '>': return '<'; case 0xAB: return 0xBB; case 0xBB: return 0xAB; } return c; }

// 3. Ordre visuel. base_rtl = direction du paragraphe (vrai pour les langues arabe/hébreu).
inline std::string visual(const std::string& utf8, bool base_rtl) {
  U32 t = shape(utf8_decode(utf8)); size_t n = t.size(); if (!n) return "";
  std::vector<Cls> cl(n); for (size_t i = 0; i < n; ++i) cl[i] = classify(t[i]);
  for (size_t i = 1; i + 1 < n; ++i) if (cl[i] == NEU && (t[i] == '.' || t[i] == ',' || t[i] == ':' || t[i] == '/') && cl[i - 1] == N && cl[i + 1] == N) cl[i] = N;   // 0.2  12:30
  std::vector<int> lv(n); const int base = base_rtl ? 1 : 0;
  // neutres : prennent la direction commune des voisins forts (chiffre = L pour cette règle), sinon celle du paragraphe
  std::vector<Cls> dir(n); for (size_t i = 0; i < n; ++i) dir[i] = cl[i] == N ? L : cl[i];
  for (size_t i = 0; i < n; ++i) if (cl[i] == NEU) { int a = int(i) - 1, b = int(i) + 1; while (a >= 0 && cl[a] == NEU) --a; while (b < int(n) && cl[b] == NEU) ++b;
      Cls da = a >= 0 ? dir[a] : (base ? R : L), db = b < int(n) ? dir[b] : (base ? R : L); dir[i] = da == db ? da : (base ? R : L); }
  for (size_t i = 0; i < n; ++i) { bool afterR = false; for (int a = int(i) - 1; a >= 0; --a) { if (cl[a] == NEU) continue; afterR = dir[a] == R; break; }
    if (dir[i] == R) lv[i] = 1; else if (cl[i] == N) lv[i] = (base || afterR) ? 2 : 0; else lv[i] = base ? 2 : 0; }
  for (size_t i = 0; i < n; ++i) if (lv[i] & 1) t[i] = mirror_char(t[i]);
  int mx = 0; for (int v : lv) mx = v > mx ? v : mx;
  for (int l = mx; l >= 1; --l) for (size_t i = 0; i < n;) { if (lv[i] < l) { ++i; continue; } size_t j = i; while (j < n && lv[j] >= l) ++j; for (size_t a = i, b = j - 1; a < b; ++a, --b) { std::swap(t[a], t[b]); std::swap(lv[a], lv[b]); } i = j; }
  return utf8_encode(t);
}
// Position du bord de départ (début de lecture) : droite en RTL, gauche en LTR.
inline float start_x(float left, float width, float text_w, bool rtl) { return rtl ? left + width - text_w : left; }

// Icônes directionnelles : en RTL on échange leur équivalent miroir (les autres — horloge, coche, cloche — ne changent pas).
inline const char* rtl_icon(const char* n, bool rtl) {
  if (!rtl) return n;
  static const char* P[][2] = {{"arrow-left", "arrow-right"}, {"arrow-right", "arrow-left"}, {"chevron-left", "chevron-right"}, {"chevron-right", "chevron-left"}, {"skip-back", "skip-forward"}, {"skip-forward", "skip-back"}, {"corner-down-left", "corner-down-right"}, {"undo", "redo"}, {"redo", "undo"}, {"log-in", "log-out"}};
  for (auto& p : P) { const char* a = p[0]; const char* b = n; while (*a && *a == *b) { ++a; ++b; } if (!*a && !*b) return p[1]; } return n;
}
}  // namespace BauraText
#endif
