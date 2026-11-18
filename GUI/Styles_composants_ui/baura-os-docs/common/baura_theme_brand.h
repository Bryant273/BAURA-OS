// BAURA OS — baura_theme_brand.h : jetons de la CHARTE (indigo / violet / lavande / or) en ARGB, clair + sombre.
// Remplace la palette bleu/cyan de GUI/UI_FRAMEWORK/baura_theme.h (qui ne correspond pas à la charte).
// Les noms reprennent les jetons CSS du prototype (fg, mut, glass, hair, hover…) pour que design et code parlent pareil.
#ifndef BAURA_THEME_BRAND_H
#define BAURA_THEME_BRAND_H
#include <cstdint>
namespace BauraBrand {
struct Tokens {
  uint32_t b1, b2, b3, b4;            // indigo, violet, lavande, or
  uint32_t fg, mut;                   // texte, texte discret
  uint32_t glass, glass_strong, glass_line, hair, hover, press, field, field_line;
  uint32_t win, win_bar, win_line;    // fenêtre
  uint32_t accent, err, ok;
  uint32_t shadow1, shadow2;          // ombres portées (--shadow)
  float blur, saturate, highlight;
  uint32_t on_accent = 0xFFFFFFFF;    // texte/icône posés sur b1/b2/err (blanc par défaut ; NOIR en contraste élevé sombre)
};
inline constexpr Tokens LIGHT{
  0xFF4F46E5, 0xFF8B5CF6, 0xFFC4B5FD, 0xFFAB8E18, 0xFF17133D, 0xFF5F5A88,
  0x57FFFFFF, 0x8CFFFFFF, 0xB3FFFFFF, 0x244F46E5, 0x1A4F46E5, 0x2E4F46E5, 0x6BFFFFFF, 0xB3FFFFFF,
  0xBDFFFFFF, 0x6BFFFFFF, 0xB3FFFFFF, 0xFF4F46E5, 0xFFD6303C, 0xFF1F8A57, 0x38342890, 0x1A342890, 36.f, 1.9f, .75f};
inline constexpr Tokens DARK{
  0xFF6D66F2, 0xFF9D77FF, 0xFFCFC3FF, 0xFFD0AE2A, 0xFFF4F2FF, 0xFFB4B0DA,
  0x5C282068, 0x9E1C164A, 0x33FFFFFF, 0x1FFFFFFF, 0x1AFFFFFF, 0x2BFFFFFF, 0x1AFFFFFF, 0x33FFFFFF,
  0xCC1A1544, 0x80221C56, 0x29FFFFFF, 0xFFB79AF5, 0xFFFF7A84, 0xFF5FD49B, 0x80000000, 0x4D000000, 36.f, 1.9f, .20f};
}
#endif
