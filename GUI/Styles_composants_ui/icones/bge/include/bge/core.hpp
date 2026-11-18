// BGE – Baura Graphical Engine · core.hpp
// Types de base, jetons de thème (miroir exact de baura-theme.css), courbes d'animation.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace bge {

struct Vec2 { float x = 0, y = 0; };
struct Rect {
  float x = 0, y = 0, w = 0, h = 0;
  bool contains(Vec2 p) const { return p.x >= x && p.y >= y && p.x < x + w && p.y < y + h; }
  Rect inset(float d) const { return {x + d, y + d, std::max(0.f, w - 2 * d), std::max(0.f, h - 2 * d)}; }
  Vec2 center() const { return {x + w / 2, y + h / 2}; }
};

// Couleur à alpha « droit » (non prémultiplié), composantes 0..1.
struct Color {
  float r = 0, g = 0, b = 0, a = 1;
  static Color hex(uint32_t rgb, float a = 1) { return {((rgb >> 16) & 255) / 255.f, ((rgb >> 8) & 255) / 255.f, (rgb & 255) / 255.f, a}; }
  static Color rgba(int r, int g, int b, float a) { return {r / 255.f, g / 255.f, b / 255.f, a}; }
  Color alpha(float k) const { return {r, g, b, a * k}; }
  static Color mix(Color p, Color q, float t) { return {p.r + (q.r - p.r) * t, p.g + (q.g - p.g) * t, p.b + (q.b - p.b) * t, p.a + (q.a - p.a) * t}; }
};

// ---- Jetons (CSS : --fg --mut --glass … ) --------------------------------------------------
struct Theme {
  bool dark = false;
  // marque
  Color b1, b2, b3, b4;                 // indigo, violet, lavande, or
  // texte
  Color fg, mut;
  // surfaces verre
  Color glass, glassStrong, glassLine;
  Color hair, hover, press, field, fieldLine;
  Color win, winBar, winLine;
  Color accent, err, ok;
  float blur = 36, sat = 1.9f;          // --blur / --sat
  // ombre « --shadow » : 0 12 34 rgba(52,40,140,.22) + 0 2 6 rgba(52,40,140,.10)
  Color shadow1, shadow2; float shadowY1 = 12, shadowB1 = 34, shadowY2 = 2, shadowB2 = 6;

  static Theme light() {
    Theme t;
    t.b1 = Color::hex(0x4F46E5); t.b2 = Color::hex(0x8B5CF6); t.b3 = Color::hex(0xC4B5FD); t.b4 = Color::hex(0xAB8E18);
    t.fg = Color::hex(0x17133d); t.mut = Color::hex(0x5f5a88);
    t.glass = Color::rgba(255, 255, 255, .34f); t.glassStrong = Color::rgba(255, 255, 255, .55f); t.glassLine = Color::rgba(255, 255, 255, .70f);
    t.hair = Color::rgba(79, 70, 229, .14f); t.hover = Color::rgba(79, 70, 229, .10f); t.press = Color::rgba(79, 70, 229, .18f);
    t.field = Color::rgba(255, 255, 255, .42f); t.fieldLine = Color::rgba(255, 255, 255, .70f);
    t.win = Color::rgba(255, 255, 255, .74f); t.winBar = Color::rgba(255, 255, 255, .42f); t.winLine = Color::rgba(255, 255, 255, .70f);
    t.accent = t.b1; t.err = Color::hex(0xd6303c); t.ok = Color::hex(0x1f8a57);
    t.shadow1 = Color::rgba(52, 40, 140, .22f); t.shadow2 = Color::rgba(52, 40, 140, .10f);
    return t;
  }
  static Theme darkTheme() {
    Theme t = light(); t.dark = true;
    t.b1 = Color::hex(0x6D66F2); t.b2 = Color::hex(0x9D77FF); t.b3 = Color::hex(0xCFC3FF); t.b4 = Color::hex(0xD0AE2A);
    t.fg = Color::hex(0xf4f2ff); t.mut = Color::hex(0xb4b0da);
    t.glass = Color::rgba(40, 32, 104, .36f); t.glassStrong = Color::rgba(28, 22, 74, .62f); t.glassLine = Color::rgba(255, 255, 255, .20f);
    t.hair = Color::rgba(255, 255, 255, .12f); t.hover = Color::rgba(255, 255, 255, .10f); t.press = Color::rgba(255, 255, 255, .17f);
    t.field = Color::rgba(255, 255, 255, .10f); t.fieldLine = Color::rgba(255, 255, 255, .20f);
    t.win = Color::rgba(26, 21, 68, .80f); t.winBar = Color::rgba(34, 28, 86, .50f); t.winLine = Color::rgba(255, 255, 255, .16f);
    t.accent = Color::mix(t.b2, Color::hex(0xffffff), .38f); t.err = Color::hex(0xff7a84); t.ok = Color::hex(0x5fd49b);
    t.shadow1 = Color::rgba(0, 0, 0, .50f); t.shadow2 = Color::rgba(0, 0, 0, .30f); t.shadowB1 = 38; t.shadowY1 = 14;
    return t;
  }
};

// ---- Courbes -------------------------------------------------------------------------------
// cubic-bezier(x1,y1,x2,y2) identique au CSS (résolution de x par Newton + bissection).
struct CubicBezier {
  float x1, y1, x2, y2;
  float operator()(float x) const {
    if (x <= 0) return 0; if (x >= 1) return 1;
    auto bx = [&](float t) { float u = 1 - t; return 3 * u * u * t * x1 + 3 * u * t * t * x2 + t * t * t; };
    auto by = [&](float t) { float u = 1 - t; return 3 * u * u * t * y1 + 3 * u * t * t * y2 + t * t * t; };
    float lo = 0, hi = 1, t = x;
    for (int i = 0; i < 24; ++i) { float v = bx(t); if (std::fabs(v - x) < 1e-5f) break; (v < x ? lo : hi) = t; t = (lo + hi) / 2; }
    return by(t);
  }
};
inline const CubicBezier SPRING{.3f, 1.45f, .45f, 1.f};     // --spring (dépasse 1 puis revient)
inline const CubicBezier EASE_OUT{.16f, 1.f, .3f, 1.f};      // --ease-out
inline const CubicBezier EASE{.25f, .1f, .25f, 1.f};

// ---- Animations ----------------------------------------------------------------------------
struct Tween {
  float from = 0, to = 1, dur = .3f, t = 0, delay = 0; CubicBezier ease = EASE_OUT;
  std::function<void(float)> apply; std::function<void()> done; bool finished = false;
  void step(float dt) {
    if (finished) return;
    if (delay > 0) { delay -= dt; return; }
    t += dt; float k = std::min(1.f, t / dur);
    if (apply) apply(from + (to - from) * ease(k));
    if (k >= 1) { finished = true; if (done) done(); }
  }
};
class Animator {
 public:
  void add(Tween tw) { list_.push_back(std::move(tw)); }
  void update(float dt) {
    for (auto& tw : list_) tw.step(dt);
    list_.erase(std::remove_if(list_.begin(), list_.end(), [](const Tween& t) { return t.finished; }), list_.end());
  }
  bool busy() const { return !list_.empty(); }
 private:
  std::vector<Tween> list_;
};

}  // namespace bge
