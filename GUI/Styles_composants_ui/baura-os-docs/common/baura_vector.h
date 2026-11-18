// BAURA OS — baura_vector.h
// Primitives VECTORIELLES anti-crénelées au-dessus du BGERenderer EXISTANT (GUI/BGE/renderer.h) :
// rectangles arrondis (SDF), chemins SVG (remplissage + trait), icônes, symbole BAURA, dégradés.
// Les couleurs sont des ARGB 0xAARRGGBB comme partout dans le dépôt. Aucune modification de GUI/BGE.
#ifndef BAURA_VECTOR_H
#define BAURA_VECTOR_H
#include "renderer.h"
#include "icons_data.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace BauraVec {
struct Vec2 { float x = 0, y = 0; };
struct Rect { float x = 0, y = 0, w = 0, h = 0; Vec2 center() const { return {x + w / 2, y + h / 2}; } Rect inset(float d) const { return {x + d, y + d, std::max(0.f, w - 2 * d), std::max(0.f, h - 2 * d)}; } };

inline uint32_t argb(int a, int r, int g, int b) { return (uint32_t(a) << 24) | (uint32_t(r) << 16) | (uint32_t(g) << 8) | uint32_t(b); }
inline uint32_t with_alpha(uint32_t c, float k) { int a = int(((c >> 24) & 255) * k + .5f); return (uint32_t(std::clamp(a, 0, 255)) << 24) | (c & 0xFFFFFF); }
inline uint32_t mix(uint32_t p, uint32_t q, float t) {
  auto ch = [&](int s) { return int(((p >> s) & 255) + (int((q >> s) & 255) - int((p >> s) & 255)) * t + .5f); };
  return argb(ch(24), ch(16), ch(8), ch(0));
}
// Peinture : unie ou dégradé linéaire 2 arrêts (angle CSS : 0° = vers le haut, 180° = vers le bas)
struct Paint {
  uint32_t c0 = 0, c1 = 0; float angle = 180; bool grad = false;
  Paint() {}
  Paint(uint32_t c) : c0(c) {}
  static Paint linear(float deg, uint32_t a, uint32_t b) { Paint p; p.c0 = a; p.c1 = b; p.angle = deg; p.grad = true; return p; }
  uint32_t at(const Rect& box, float x, float y) const {
    if (!grad) return c0;
    float a = angle * 3.14159265f / 180, dx = std::sin(a), dy = -std::cos(a), len = std::fabs(box.w * dx) + std::fabs(box.h * dy);
    if (len < 1e-3f) return c0;
    Vec2 c = box.center();
    return mix(c0, c1, std::clamp(((x - c.x) * dx + (y - c.y) * dy) / len + .5f, 0.f, 1.f));
  }
};

struct Path {
  struct Sub { std::vector<Vec2> pts; bool closed = false; };
  std::vector<Sub> subs;

  static Path fromSVG(const char* d, int curveSteps = 28) { Path p; p.parse(d, curveSteps); return p; }

 private:
  Vec2 cur{}, start{}, lastC{}; char lastCmd = 0; int N = 28;
  void mv(Vec2 p) { subs.push_back({}); subs.back().pts.push_back(p); cur = start = p; }
  void ln(Vec2 p) { if (subs.empty()) mv(cur); subs.back().pts.push_back(p); cur = p; }
  void cubic(Vec2 a, Vec2 b, Vec2 c) {
    Vec2 p0 = cur;
    for (int i = 1; i <= N; ++i) {
      float t = float(i) / N, u = 1 - t;
      ln({u * u * u * p0.x + 3 * u * u * t * a.x + 3 * u * t * t * b.x + t * t * t * c.x,
          u * u * u * p0.y + 3 * u * u * t * a.y + 3 * u * t * t * b.y + t * t * t * c.y});
    }
    lastC = b;
  }
  void quad(Vec2 a, Vec2 c) { Vec2 p0 = cur; cubic({p0.x + 2.f / 3 * (a.x - p0.x), p0.y + 2.f / 3 * (a.y - p0.y)}, {c.x + 2.f / 3 * (a.x - c.x), c.y + 2.f / 3 * (a.y - c.y)}, c); lastC = a; }
  // Arc SVG (spec F.6.5) : conversion « endpoint » → « centre » puis échantillonnage
  void arc(float rx, float ry, float rotDeg, bool large, bool sweep, Vec2 e) {
    Vec2 s = cur; if (rx == 0 || ry == 0 || (s.x == e.x && s.y == e.y)) { ln(e); return; }
    rx = std::fabs(rx); ry = std::fabs(ry);
    float phi = rotDeg * 3.14159265f / 180, cp = std::cos(phi), sp = std::sin(phi);
    float dx = (s.x - e.x) / 2, dy = (s.y - e.y) / 2;
    float x1 = cp * dx + sp * dy, y1 = -sp * dx + cp * dy;
    float lam = x1 * x1 / (rx * rx) + y1 * y1 / (ry * ry); if (lam > 1) { float k = std::sqrt(lam); rx *= k; ry *= k; }
    float num = rx * rx * ry * ry - rx * rx * y1 * y1 - ry * ry * x1 * x1, den = rx * rx * y1 * y1 + ry * ry * x1 * x1;
    float co = std::sqrt(std::max(0.f, num / den)) * (large == sweep ? -1.f : 1.f);
    float cx1 = co * rx * y1 / ry, cy1 = -co * ry * x1 / rx;
    float cx = cp * cx1 - sp * cy1 + (s.x + e.x) / 2, cy = sp * cx1 + cp * cy1 + (s.y + e.y) / 2;
    auto ang = [](float ux, float uy, float vx, float vy) { float a = std::atan2(ux * vy - uy * vx, ux * vx + uy * vy); return a; };
    float th1 = ang(1, 0, (x1 - cx1) / rx, (y1 - cy1) / ry);
    float dth = ang((x1 - cx1) / rx, (y1 - cy1) / ry, (-x1 - cx1) / rx, (-y1 - cy1) / ry);
    if (!sweep && dth > 0) dth -= 6.2831853f; else if (sweep && dth < 0) dth += 6.2831853f;
    int n = std::max(8, int(std::fabs(dth) / 1.5707963f * N));
    for (int i = 1; i <= n; ++i) {
      float a = th1 + dth * i / n, ca = std::cos(a), sa = std::sin(a);
      ln({cp * rx * ca - sp * ry * sa + cx, sp * rx * ca + cp * ry * sa + cy});
    }
    cur = e;
  }
  static bool num(const char*& p, float& out) {
    while (*p == ' ' || *p == ',' || *p == '\n' || *p == '\t') ++p;
    const char* b = p; if (*p == '-' || *p == '+') ++p;
    while (*p >= '0' && *p <= '9') ++p;
    if (*p == '.') { ++p; while (*p >= '0' && *p <= '9') ++p; }
    if ((*p == 'e' || *p == 'E') && (p[1] == '-' || p[1] == '+' || (p[1] >= '0' && p[1] <= '9'))) {
      p += 2;
      while (*p >= '0' && *p <= '9')
        ++p;
    }
    if (p == b) return false;
    out = std::strtof(b, nullptr);
    return true;
  }
  static bool flag(const char*& p, bool& out) {
    while (*p == ' ' || *p == ',')
      ++p;
    if (*p != '0' && *p != '1') return false;
    out = *p++ == '1';
    return true;
  }
  void parse(const char* p, int steps) {
    N = steps; char cmd = 0;
    while (true) {
      while (*p == ' ' || *p == ',' || *p == '\n') ++p;
      if (!*p) break;
      if ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z')) cmd = *p++;
      else if (cmd == 'M') cmd = 'L'; else if (cmd == 'm') cmd = 'l';  // paires implicites
      bool rel = cmd >= 'a'; char C = rel ? char(cmd - 32) : cmd; float v[7]; bool fl, fs;
      Vec2 o = rel ? cur : Vec2{0, 0};
      auto need = [&](int n) { for (int i = 0; i < n; ++i) if (!num(p, v[i])) return false; return true; };
      if (C == 'Z') { if (!subs.empty()) { subs.back().closed = true; cur = start; } lastCmd = 'Z'; cmd = 0; continue; }
      if (C == 'M') { if (!need(2)) break; mv({o.x + v[0], o.y + v[1]}); }
      else if (C == 'L') { if (!need(2)) break; ln({o.x + v[0], o.y + v[1]}); }
      else if (C == 'H') { if (!need(1)) break; ln({(rel ? cur.x : 0) + v[0], cur.y}); }
      else if (C == 'V') { if (!need(1)) break; ln({cur.x, (rel ? cur.y : 0) + v[0]}); }
      else if (C == 'C') { if (!need(6)) break; cubic({o.x + v[0], o.y + v[1]}, {o.x + v[2], o.y + v[3]}, {o.x + v[4], o.y + v[5]}); }
      else if (C == 'S') { if (!need(4)) break; Vec2 a = (lastCmd == 'C' || lastCmd == 'S') ? Vec2{2 * cur.x - lastC.x, 2 * cur.y - lastC.y} : cur; cubic(a, {o.x + v[0], o.y + v[1]}, {o.x + v[2], o.y + v[3]}); }
      else if (C == 'Q') { if (!need(4)) break; quad({o.x + v[0], o.y + v[1]}, {o.x + v[2], o.y + v[3]}); }
      else if (C == 'T') { if (!need(2)) break; Vec2 a = (lastCmd == 'Q' || lastCmd == 'T') ? Vec2{2 * cur.x - lastC.x, 2 * cur.y - lastC.y} : cur; quad(a, {o.x + v[0], o.y + v[1]}); }
      else if (C == 'A') { float e[2]; if (!need(3) || !flag(p, fl) || !flag(p, fs) || !num(p, e[0]) || !num(p, e[1])) break; arc(v[0], v[1], v[2], fl, fs, {o.x + e[0], o.y + e[1]}); }
      else break;
      lastCmd = C;
    }
  }
};
// ---------------------------------------------------------------------------------------------
// Peintre : écrit dans un BGERenderer (alpha « source over » via BGERenderer::blend_pixels)
// ---------------------------------------------------------------------------------------------
class Painter {
 public:
  explicit Painter(BGERenderer& r) : rd(r), cw(int(r.framebuffer().width)), ch(int(r.framebuffer().height)) { cx1 = cw; cy1 = ch; }
  BGERenderer& renderer() { return rd; }
  void set_clip(int x, int y, int w, int h) { cx0 = std::max(0, x); cy0 = std::max(0, y); cx1 = std::min(cw, x + w); cy1 = std::min(ch, y + h); }
  int width() const { return cw; }
  int height() const { return ch; }
  void reset_clip() { cx0 = 0; cy0 = 0; cx1 = cw; cy1 = ch; }
  void set_opacity(float k) { op = std::clamp(k, 0.f, 1.f); }   // multiplie toute écriture (fondus de fenêtres)
  float opacity() const { return op; }
  void put(int x, int y, uint32_t c, float cov) {
    if (x < cx0 || y < cy0 || x >= cx1 || y >= cy1) return;
    int a = int(((c >> 24) & 255) * cov * op + .5f); if (a <= 0) return;
    rd.draw_pixel_alpha(x, y, c & 0xFFFFFF, uint8_t(std::min(255, a)));
  }
  // --- rectangles arrondis par distance signée (bord anti-crénelé à tout rayon) ---
  static float sd_rr(const Rect& r, float rad, float x, float y) {
    rad = std::min(rad, std::min(r.w, r.h) / 2); float qx = std::fabs(x - r.x - r.w / 2) - (r.w / 2 - rad), qy = std::fabs(y - r.y - r.h / 2) - (r.h / 2 - rad);
    return std::sqrt(std::max(qx, 0.f) * std::max(qx, 0.f) + std::max(qy, 0.f) * std::max(qy, 0.f)) + std::min(std::max(qx, qy), 0.f) - rad;
  }
  static float cov_rr(const Rect& r, float rad, float x, float y) { return std::clamp(.5f - sd_rr(r, rad, x, y), 0.f, 1.f); }
  void fill_rrect(Rect r, float rad, const Paint& p) {
    for (int y = int(std::floor(r.y)) - 1; y <= int(std::ceil(r.y + r.h)); ++y) for (int x = int(std::floor(r.x)) - 1; x <= int(std::ceil(r.x + r.w)); ++x) {
      float c = cov_rr(r, rad, x + .5f, y + .5f); if (c > 0) put(x, y, p.at(r, x + .5f, y + .5f), c); }
  }
  void stroke_rrect(Rect r, float rad, float w, const Paint& p) {  // trait intérieur (comme border CSS)
    for (int y = int(std::floor(r.y)) - 1; y <= int(std::ceil(r.y + r.h)); ++y) for (int x = int(std::floor(r.x)) - 1; x <= int(std::ceil(r.x + r.w)); ++x) {
      float c = cov_rr(r, rad, x + .5f, y + .5f) - cov_rr(r.inset(w), std::max(0.f, rad - w), x + .5f, y + .5f); if (c > 0) put(x, y, p.at(r, x + .5f, y + .5f), c); }
  }
  // --- chemins ---
  void fill_path(const Path& p, Vec2 pos, float sc, const Paint& paint) {
    float miny = 1e9f, maxy = -1e9f, minx = 1e9f, maxx = -1e9f;
    for (auto& s : p.subs) for (auto& q : s.pts) { miny = std::min(miny, pos.y + q.y * sc); maxy = std::max(maxy, pos.y + q.y * sc); minx = std::min(minx, pos.x + q.x * sc); maxx = std::max(maxx, pos.x + q.x * sc); }
    Rect box{minx, miny, maxx - minx, maxy - miny}; const int SS = 4; std::vector<float> row(size_t(cw) + 1); struct X { float x; int d; }; std::vector<X> xs;
    for (int y = std::max(0, int(miny)); y <= std::min(ch - 1, int(maxy)); ++y) {
      std::fill(row.begin(), row.end(), 0.f);
      for (int k = 0; k < SS; ++k) {
        float sy = y + (k + .5f) / SS; xs.clear();
        for (auto& s : p.subs) { size_t n = s.pts.size(); if (n < 2) continue;
          for (size_t i = 0; i < n; ++i) { Vec2 a = s.pts[i], b = s.pts[(i + 1) % n]; float ay = pos.y + a.y * sc, by = pos.y + b.y * sc; if (ay == by) continue;
            if ((sy >= ay) != (sy >= by)) { float t = (sy - ay) / (by - ay); xs.push_back({pos.x + a.x * sc + (b.x - a.x) * sc * t, by > ay ? 1 : -1}); } } }
        std::sort(xs.begin(), xs.end(), [](const X& a, const X& b) { return a.x < b.x; });
        int wind = 0; for (size_t i = 0; i + 1 < xs.size(); ++i) { wind += xs[i].d; if (wind != 0) { float xa = std::max(0.f, xs[i].x), xb = std::min(float(cw), xs[i + 1].x);
          for (int px = int(xa); px < xb && px < cw; ++px) row[px] += std::max(0.f, std::min(xb, px + 1.f) - std::max(xa, float(px))) / SS; } }
      }
      for (int x = 0; x < cw; ++x) if (row[x] > 0) put(x, y, paint.at(box, x + .5f, y + .5f), std::min(1.f, row[x]));
    }
  }
  void stroke_path(const Path& p, Vec2 pos, float sc, float width, const Paint& paint) {  // bouts et jointures ronds
    float hw = width * sc / 2; struct Seg { Vec2 a, b; }; std::vector<Seg> segs; float minx = 1e9f, miny = 1e9f, maxx = -1e9f, maxy = -1e9f;
    for (auto& s : p.subs) { size_t n = s.pts.size(); size_t m = s.closed ? n : n - 1; if (n == 1) segs.push_back({s.pts[0], s.pts[0]}); for (size_t i = 0; i < m && n > 1; ++i) segs.push_back({s.pts[i], s.pts[(i + 1) % n]}); }
    for (auto& s : segs) for (Vec2 q : {s.a, s.b}) { minx = std::min(minx, pos.x + q.x * sc); maxx = std::max(maxx, pos.x + q.x * sc); miny = std::min(miny, pos.y + q.y * sc); maxy = std::max(maxy, pos.y + q.y * sc); }
    Rect box{minx, miny, maxx - minx, maxy - miny};
    for (int y = int(miny - hw - 1); y <= int(maxy + hw + 1); ++y) for (int x = int(minx - hw - 1); x <= int(maxx + hw + 1); ++x) {
      float px = x + .5f, py = y + .5f, best = 1e9f;
      for (auto& s : segs) { float ax = pos.x + s.a.x * sc, ay = pos.y + s.a.y * sc, dx = pos.x + s.b.x * sc - ax, dy = pos.y + s.b.y * sc - ay, L = dx * dx + dy * dy;
        float t = L > 0 ? std::clamp(((px - ax) * dx + (py - ay) * dy) / L, 0.f, 1.f) : 0; float ex = ax + dx * t - px, ey = ay + dy * t - py; best = std::min(best, ex * ex + ey * ey); }
      float c = std::clamp(hw - std::sqrt(best) + .5f, 0.f, 1.f); if (c > 0) put(x, y, paint.at(box, px, py), c); }
  }
  // Icône 24×24 (trait 1,9, bouts ronds) dans r
  bool icon(const char* name, Rect r, uint32_t color, float stroke = 1.9f) {
    for (size_t a = 0; a < icons::ALIAS_COUNT; ++a) if (!std::strcmp(icons::ALIASES[a].from, name)) { name = icons::ALIASES[a].to; break; }  // ancien nom / id Papirus
    for (size_t i = 0; i < icons::COUNT; ++i) if (!std::strcmp(icons::ALL[i].name, name)) {
      float sc = std::min(r.w, r.h) / 24.f; Vec2 pos{r.x + (r.w - 24 * sc) / 2, r.y + (r.h - 24 * sc) / 2};
      for (size_t k = 0; k < icons::ALL[i].n; ++k) { Path pa = Path::fromSVG(icons::ALL[i].elems[k].d, 16); if (icons::ALL[i].elems[k].fill) fill_path(pa, pos, sc, color); stroke_path(pa, pos, sc, stroke, color); }
      return true; }
#ifdef BAURA_ICON_DEBUG
    std::fprintf(stderr, "icone manquante: %s\n", name);
#endif
    return false;
  }
  // Symbole BAURA (viewBox 150 29 199 420) : barre + deux demi-disques + point doré
  void symbol(Rect r, uint32_t c1, uint32_t c2, uint32_t c3, uint32_t c4) {
    float sc = std::min(r.w / 199.f, r.h / 420.f); Vec2 pos{r.x + (r.w - 199 * sc) / 2 - 150 * sc, r.y + (r.h - 420 * sc) / 2 - 29 * sc};
    static const Path bar = Path::fromSVG("M174 29A24 24 0 0 1 198 53V425A24 24 0 0 1 150 425V53A24 24 0 0 1 174 29Z", 24), top = Path::fromSVG("M248 29A101 102.5 0 0 1 248 234Z", 64),
      bot = Path::fromSVG("M248 245A101 102 0 0 1 248 449Z", 64), dot = Path::fromSVG("M331 221H334A15 15 0 0 1 349 236V242A15 15 0 0 1 334 257H331A15 15 0 0 1 316 242V236A15 15 0 0 1 331 221Z", 16);
    fill_path(bar, pos, sc, c1); fill_path(top, pos, sc, c2); fill_path(bot, pos, sc, c3); fill_path(dot, pos, sc, c4);
  }
 private:
  BGERenderer& rd; int cw, ch, cx0 = 0, cy0 = 0, cx1, cy1; float op = 1.f;
 public:
  struct Init {};
};
}  // namespace BauraVec
#endif
