// BAURA OS — baura_wm_motion.h : compositeur ANIMÉ qui observe BGEWindowManager (aucun changement d'API hors getter, voir patches/).
// Ouverture, fermeture, réduction vers la barre des tâches, restauration, agrandissement, effet « déplacement ».
#ifndef BAURA_WM_MOTION_H
#define BAURA_WM_MOTION_H
#include "baura_motion.h"
#include "baura_ui.h"
#include "window_manager.h"   // GUI/BGE/window_manager.h patché (windows_view, dragging)
#include <functional>
#include <map>

namespace BauraUI {
using namespace BauraMotion;
class WindowMotion {
 public:
  struct Visual {
    enum Mode { Opening, Idle, Closing, Minimizing, Minimized, Restoring } mode = Opening;
    float t = 0; std::string title; BauraVec::Rect bounds, from, to; bool focused = false, maximized = false, minimized_seen = false;
    Spring scale, opacity; const char* icon = "app-window"; uint32_t c1 = 0xFF6D66F2, c2 = 0xFF4F46E5; BauraVec::Rect dock; int z = 0;
    float k_open = 0;   // progression de la courbe
  };
  bool reduce_motion = false;
  // pour chaque fenêtre : l'icône de la barre des tâches (cible de la réduction) et son style
  std::function<BauraVec::Rect(uint32_t)> dock_rect = [](uint32_t) { return BauraVec::Rect{40, 560, 40, 40}; };
  std::function<void(uint32_t, Visual&)> style = [](uint32_t, Visual&) {};

  void update(const BGEWindowManager& wm, float dt) {
    const auto& ws = wm.windows_view(); std::map<uint32_t, bool> present; int z = 0;
    for (const BGEWindow& w : ws) { present[w.id] = true; Visual& v = vis[w.id]; BauraVec::Rect b{float(w.bounds.x), float(w.bounds.y), float(w.bounds.width), float(w.bounds.height)};
      if (v.title.empty()) { v.title = w.title; v.bounds = v.to = b; v.dock = dock_rect(w.id); style(w.id, v); v.scale.set(.94f); v.opacity.set(0); v.mode = Visual::Opening; v.t = 0; }
      v.title = w.title; v.focused = w.focused; v.z = z++; v.dock = dock_rect(w.id);
      if (w.minimized && v.mode != Visual::Minimized && v.mode != Visual::Minimizing) { v.mode = Visual::Minimizing; v.t = 0; v.from = v.bounds; }
      else if (!w.minimized && (v.mode == Visual::Minimized || v.mode == Visual::Minimizing)) { v.mode = Visual::Restoring; v.t = 0; v.from = v.bounds; }
      if (w.maximized != v.maximized) { v.maximized = w.maximized; v.from = v.bounds; v.to = b; v.t = 0; max_anim[w.id] = 0; }
      if (!max_anim.count(w.id)) v.bounds = b;
      v.to = b;
      bool drag = wm.dragging() == w.id; v.scale.target = (drag ? .97f : 1.f); v.opacity.target = drag ? .88f : 1.f;
    }
    for (auto& kv : vis) if (!present.count(kv.first) && kv.second.mode != Visual::Closing) { kv.second.mode = Visual::Closing; kv.second.t = 0; }
    for (auto it = vis.begin(); it != vis.end();) { Visual& v = it->second; float d = reduce_motion ? 1e3f : dt; v.t += d;
      switch (v.mode) {
        case Visual::Opening: { float k = SPRING(std::min(1.f, v.t / Dur::WindowOpen)); v.scale.pos = .94f + .06f * k; v.opacity.pos = std::min(1.f, v.t / Dur::WindowOpen * 3); if (v.t >= Dur::WindowOpen) { v.mode = Visual::Idle; v.scale.set(1); v.opacity.set(1); } break; }
        case Visual::Idle: v.scale.step(dt); v.opacity.step(dt); break;
        case Visual::Closing: if (v.t >= Dur::WindowClose) { it = vis.erase(it); continue; } break;
        case Visual::Minimizing: if (v.t >= Dur::Minimize) { v.mode = Visual::Minimized; } break;
        case Visual::Restoring: if (v.t >= Dur::Minimize) { v.mode = Visual::Idle; v.scale.set(1); v.opacity.set(1); } break;
        case Visual::Minimized: break; }
      auto m = max_anim.find(it->first); if (m != max_anim.end()) { m->second += d; float k = EASE_IN_OUT(std::min(1.f, m->second / Dur::Maximize));
        v.bounds = {v.from.x + (v.to.x - v.from.x) * k, v.from.y + (v.to.y - v.from.y) * k, v.from.w + (v.to.w - v.from.w) * k, v.from.h + (v.to.h - v.from.h) * k}; if (m->second >= Dur::Maximize) { max_anim.erase(m); v.bounds = v.to; } }
      ++it; }
  }
  // Rectangle et opacité effectivement dessinés pour une fenêtre
  bool drawn(const Visual& v, BauraVec::Rect& r, float& op) const {
    using BauraVec::Rect; r = v.bounds; op = 1; float s = v.scale.pos;
    switch (v.mode) {
      case Visual::Opening: case Visual::Idle: op = v.opacity.pos; break;
      case Visual::Closing: { float k = EASE_IN(std::min(1.f, v.t / Dur::WindowClose)); s = 1 - .04f * k; op = 1 - k; break; }
      case Visual::Minimizing: case Visual::Restoring: { float k = EASE_IN_OUT(std::min(1.f, v.t / Dur::Minimize)); if (v.mode == Visual::Restoring) k = 1 - k;
        BauraVec::Vec2 c = v.from.center(), d{v.dock.x + v.dock.w / 2, v.dock.y + v.dock.h / 2}; float sc = 1 - .88f * k; r = {c.x + (d.x - c.x) * k - v.from.w * sc / 2, c.y + (d.y - c.y) * k - v.from.h * sc / 2, v.from.w * sc, v.from.h * sc}; op = 1 - k * k; return op > .01f; }
      case Visual::Minimized: return false; }
    BauraVec::Vec2 c = r.center(); r = {c.x - r.w * s / 2, c.y - r.h * s / 2, r.w * s, r.h * s}; return op > .01f;
  }
  // Dessine toutes les fenêtres visibles dans l'ordre Z ; `content(rect_utile)` dessine l'intérieur.
  void draw(Ui& u, const std::function<void(uint32_t, BauraVec::Rect)>& content) {
    std::vector<std::pair<int, uint32_t>> order; for (auto& kv : vis) order.push_back({kv.second.mode == Visual::Closing ? 1000 : kv.second.z, kv.first}); std::sort(order.begin(), order.end());
    for (auto& o : order) { Visual& v = vis[o.second]; BauraVec::Rect r; float op; if (!drawn(v, r, op)) continue; u.p.set_opacity(op);
      BauraVec::Rect body = window_frame(u, r, v.title, v.icon, v.c1, v.c2, v.focused); u.p.set_clip(int(r.x), int(r.y + 46), int(r.w), int(r.h - 46)); content(o.second, body); u.p.reset_clip(); u.p.set_opacity(1); }
  }
  size_t count() const { return vis.size(); }
  const Visual* find(uint32_t id) const { auto it = vis.find(id); return it == vis.end() ? nullptr : &it->second; }
 private:
  std::map<uint32_t, Visual> vis; std::map<uint32_t, float> max_anim;
};
}  // namespace BauraUI
#endif
