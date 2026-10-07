#ifndef BAURA_THEME_H
#define BAURA_THEME_H

#include <cstdint>

struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

namespace BauraTheme {
static constexpr uint32_t Background = 0xFF101827u;
static constexpr uint32_t Surface = 0xE8243042u;
static constexpr uint32_t SurfaceRaised = 0xF02C3B4Fu;
static constexpr uint32_t TextPrimary = 0xFFF4F7FBu;
static constexpr uint32_t TextSecondary = 0xFFB7C2D0u;
static constexpr uint32_t AccentBlue = 0xFF4A9EFFu;
static constexpr uint32_t AccentCyan = 0xFF6BD5E1u;
static constexpr uint32_t AccentPurple = 0xFF9B8AFB;
static constexpr uint32_t Success = 0xFF55C99Au;
static constexpr uint32_t Warning = 0xFFFFC66Du;
static constexpr uint32_t Danger = 0xFFFF6878u;
static constexpr uint32_t GlassLight = 0xBFFFFFFFu;
static constexpr uint32_t GlassDark = 0xC0182432u;
static constexpr uint32_t Border = 0x557E96B2u;
static constexpr uint32_t Shadow = 0x70030A12u;
static constexpr uint32_t White = 0xFFFFFFFFu;
static constexpr uint32_t Black = 0xFF000000u;

inline Color rgba(uint32_t argb) {
    return {static_cast<uint8_t>((argb >> 16) & 0xFFu),
            static_cast<uint8_t>((argb >> 8) & 0xFFu),
            static_cast<uint8_t>(argb & 0xFFu),
            static_cast<uint8_t>((argb >> 24) & 0xFFu)};
}
}

#endif
