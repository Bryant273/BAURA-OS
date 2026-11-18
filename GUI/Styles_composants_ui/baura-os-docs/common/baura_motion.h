// BAURA OS — baura_motion.h : courbes, ressort physique, tweens, animateur ; respecte « réduire les animations ».
#ifndef BAURA_MOTION_H
#define BAURA_MOTION_H
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <vector>

namespace BauraMotion {
// cubic-bezier(x1,y1,x2,y2) identique au CSS
struct Bezier {
  float x1, y1, x2, y2;
  float operator()(float x) const {
    if (x <= 0) return 0;
    if (x >= 1) return 1;
    auto bx = [&](float t) { float u = 1 - t; return 3 * u * u * t * x1 + 3 * u * t * t * x2 + t * t * t; };
    auto by = [&](float t) { float u = 1 - t; return 3 * u * u * t * y1 + 3 * u * t * t * y2 + t * t * t; };
    float lo = 0, hi = 1, t = x;
    for (int i = 0; i < 24; ++i) { float v = bx(t); if (std::fabs(v - x) < 1e-5f) break; (v < x ? lo : hi) = t; t = (lo + hi) / 2; }
    return by(t);
  }
};
inline constexpr Bezier SPRING{.3f, 1.45f, .45f, 1.f};   // « --spring » : dépasse 1 puis revient (ouvertures, pastilles)
inline constexpr Bezier EASE_OUT{.16f, 1.f, .3f, 1.f};   // entrées de panneaux
inline constexpr Bezier EASE_IN_OUT{.65f, 0.f, .35f, 1.f};  // déplacements de fenêtre
inline constexpr Bezier EASE_IN{.5f, 0.f, 1.f, .5f};     // sorties (fermeture, réduction)

// Durées normalisées (secondes)
namespace Dur { inline constexpr float Hover = .15f, Panel = .30f, WindowOpen = .30f, WindowClose = .18f, Minimize = .26f, Maximize = .28f, Toast = .35f; }

// Ressort amorti (intégration semi-implicite, pas fixe) : pour les valeurs INTERROMPABLES (poignée de curseur, fenêtre suivie à la souris)
struct Spring {
  float pos = 0, vel = 0, target = 0, stiffness = 380, damping = 26;   // ≈ amorti critique léger
  void set(float v) { pos = target = v; vel = 0; }
  void step(float dt) { const float h = 1.f / 240; float left = dt; while (left > 0) { float s = std::min(h, left); float a = -stiffness * (pos - target) - damping * vel; vel += a * s; pos += vel * s; left -= s; } }
  bool settled(float eps = .001f) const { return std::fabs(pos - target) < eps && std::fabs(vel) < eps; }
};

struct Tween {
  float from = 0, to = 1, dur = .3f, t = 0, delay = 0; Bezier ease = EASE_OUT; std::function<void(float)> apply; std::function<void()> done; bool finished = false;
  void step(float dt) { if (finished) return; if (delay > 0) { delay -= dt; return; } t += dt; float k = std::min(1.f, t / std::max(1e-4f, dur)); if (apply) apply(from + (to - from) * ease(k)); if (k >= 1) { finished = true; if (done) done(); } }
};
class Animator {
 public:
  bool reduce_motion = false;   // préférence système : durées nulles (fin immédiate)
  void add(Tween tw) { if (reduce_motion) { tw.t = tw.dur; } list.push_back(std::move(tw)); if (reduce_motion) update(0); }
  void update(float dt) { for (size_t i = 0; i < list.size(); ++i) list[i].step(dt); list.erase(std::remove_if(list.begin(), list.end(), [](const Tween& t) { return t.finished; }), list.end()); }
  bool busy() const { return !list.empty(); }
 private:
  std::vector<Tween> list;
};
}  // namespace BauraMotion
#endif
