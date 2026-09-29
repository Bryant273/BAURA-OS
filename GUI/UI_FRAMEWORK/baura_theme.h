
#ifndef BAURA_THEME_H
#define BAURA_THEME_H
#include <stdint.h>
struct Color { uint8_t r, g, b, a; };
namespace BauraTheme {
    const Color AccentBlue    = {0, 120, 215, 255};
    const Color GlassLight    = {255, 255, 255, 180};
    const Color GlassDark     = {30, 30, 30, 180};
    const Color Background    = {240, 240, 240, 255};
}
#endif
