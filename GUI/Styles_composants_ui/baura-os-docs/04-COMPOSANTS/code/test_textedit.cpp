// Tests unitaires du modèle d'édition (accents, emoji, sélection, mots). Compiler : g++ -std=c++17 test_textedit.cpp && ./a.out
#include "baura_textedit.h"
#include <cassert>
#include <cstdio>
using BauraUI::TextEdit;
int main() {
  TextEdit t; t.insert("Paramètres"); assert(t.text() == "Paramètres" && t.caret() == 11);   // è = 2 octets
  t.left(); t.left(); t.left(); t.left(); t.left(); t.left(); assert(t.caret() == 4);   // « Para| mètres »
  t.backspace(); assert(t.text() == "Parmètres");                                        // efface un caractère entier
  t.end(); t.left(true, true); assert(t.selected() == "Parmètres"); t.insert("Écran"); assert(t.text() == "Écran" && t.caret() == 6);
  t.insert(" 日本"); assert(t.text() == "Écran 日本"); t.backspace(); assert(t.text() == "Écran 日");  // 3 octets
  t.home(); t.right(false, true); assert(t.caret() == 7);                                  // saut de mot
  t.select_all(); assert(t.cut() == "Écran 日" && t.text().empty());
  TextEdit small(4); assert(small.insert("abcd") && !small.insert("e") && small.text() == "abcd");   // limite d'octets
  TextEdit u; u.insert("a"); u.del(); assert(u.text() == "a"); u.home(); u.del(); assert(u.text().empty());
  std::puts("TextEdit : tous les tests passent");
}
