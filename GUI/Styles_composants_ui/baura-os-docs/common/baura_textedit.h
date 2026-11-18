// BAURA OS — baura_textedit.h : modèle d'édition de texte (UTF-8) pour les champs de saisie.
// Logique pure, sans dessin : curseur, sélection, insertion, effacement, déplacements par caractère/mot, presse-papiers.
// Les positions sont des OFFSETS D'OCTETS toujours alignés sur une frontière de point de code.
#ifndef BAURA_TEXTEDIT_H
#define BAURA_TEXTEDIT_H
#include <algorithm>
#include <cstdint>
#include <string>

namespace BauraUI {
class TextEdit {
 public:
  explicit TextEdit(size_t max_bytes = 1024) : limit(max_bytes) {}
  const std::string& text() const { return buf; }
  size_t caret() const { return cur; }
  bool has_selection() const { return anchor != cur; }
  size_t sel_begin() const { return std::min(anchor, cur); }
  size_t sel_end() const { return std::max(anchor, cur); }
  std::string selected() const { return buf.substr(sel_begin(), sel_end() - sel_begin()); }
  void set_text(const std::string& s) { buf = s.substr(0, limit); cur = anchor = buf.size(); }
  void select_all() { anchor = 0; cur = buf.size(); }
  // Insère du texte UTF-8 (remplace la sélection). Refuse silencieusement si la limite serait dépassée.
  bool insert(const std::string& s) {
    erase_selection(); if (buf.size() + s.size() > limit) return false; buf.insert(cur, s); cur += s.size(); anchor = cur; return true;
  }
  void backspace() { if (erase_selection()) return; if (cur == 0) return; size_t p = prev(cur); buf.erase(p, cur - p); cur = anchor = p; }
  void del() { if (erase_selection()) return; if (cur >= buf.size()) return; size_t n = next(cur); buf.erase(cur, n - cur); anchor = cur; }
  void left(bool extend = false, bool word = false) { size_t p = has_selection() && !extend ? sel_begin() : (word ? word_prev(cur) : prev(cur)); move(p, extend); }
  void right(bool extend = false, bool word = false) { size_t p = has_selection() && !extend ? sel_end() : (word ? word_next(cur) : next(cur)); move(p, extend); }
  void home(bool extend = false) { move(0, extend); }
  void end(bool extend = false) { move(buf.size(), extend); }
  std::string cut() { std::string s = selected(); erase_selection(); return s; }
 private:
  std::string buf; size_t cur = 0, anchor = 0, limit;
  static bool cont(unsigned char c) { return (c & 0xC0) == 0x80; }
  size_t prev(size_t i) const { if (i == 0) return 0; do --i; while (i > 0 && cont(static_cast<unsigned char>(buf[i]))); return i; }
  size_t next(size_t i) const { if (i >= buf.size()) return buf.size(); do ++i; while (i < buf.size() && cont(static_cast<unsigned char>(buf[i]))); return i; }
  bool space(size_t i) const { return buf[i] == ' ' || buf[i] == '\t' || buf[i] == '\n'; }
  size_t word_prev(size_t i) const { while (i > 0 && space(prev(i))) i = prev(i); while (i > 0 && !space(prev(i))) i = prev(i); return i; }
  size_t word_next(size_t i) const { while (i < buf.size() && !space(i)) i = next(i); while (i < buf.size() && space(i)) i = next(i); return i; }
  void move(size_t p, bool extend) { cur = p; if (!extend) anchor = cur; }
  bool erase_selection() { if (!has_selection()) return false; size_t b = sel_begin(), e = sel_end(); buf.erase(b, e - b); cur = anchor = b; return true; }
};
}  // namespace BauraUI
#endif
