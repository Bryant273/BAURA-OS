// BGE – gfx.hpp · rasteriseur logiciel : canvas, formes arrondies, ombres, VERRE (flou d'arrière-plan),
// chemins vectoriels SVG (logo + icônes). Aucune dépendance. Remplaçable par Skia/Vulkan derrière la
// même interface Painter (voir docs/02-BGE-Architecture.md §6).
#pragma once
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "core.hpp"
#include "icons_data.hpp"

namespace bge {

// ============================================================================================
// Canvas : image RGBA flottante, alpha prémultiplié
// ============================================================================================
struct Canvas {
  int w = 0, h = 0;
  std::vector<float> px;  // 4 floats par pixel : r*a, g*a, b*a, a
  Canvas(int W, int H) : w(W), h(H), px(size_t(W) * H * 4, 0.f) {}
  float* at(int x, int y) { return &px[(size_t(y) * w + x) * 4]; }
  void over(int x, int y, const Color& c, float cov) {
    if (x < 0 || y < 0 || x >= w || y >= h) return;
    float a = c.a * cov; if (a <= 0) return;
    float* d = at(x, y), k = 1 - a;
    d[0] = c.r * a + d[0] * k; d[1] = c.g * a + d[1] * k; d[2] = c.b * a + d[2] * k; d[3] = a + d[3] * k;
  }
  bool savePPM(const char* file) const {  // sortie simple, convertible en PNG
    FILE* f = std::fopen(file, "wb"); if (!f) return false;
    std::fprintf(f, "P6\n%d %d\n255\n", w, h);
    for (size_t i = 0; i < size_t(w) * h; ++i) for (int c = 0; c < 3; ++c) {
      float v = px[i * 4 + c] + (1 - px[i * 4 + 3]);  // sur fond blanc
      std::fputc(int(std::clamp(v, 0.f, 1.f) * 255 + .5f), f);
    }
    std::fclose(f); return true;
  }
};

// ============================================================================================
// Paint : couleur unie ou dégradé linéaire à 2 arrêts (CSS linear-gradient(<angle>deg, c0, c1))
// ============================================================================================
struct Paint {
  Color c0, c1; float angle = 180; bool grad = false;
  Paint() {}
  Paint(Color c) : c0(c) {}
  static Paint linear(float deg, Color a, Color b) { Paint p; p.c0 = a; p.c1 = b; p.angle = deg; p.grad = true; return p; }
  Color at(const Rect& box, float x, float y) const {
    if (!grad) return c0;
    float a = angle * 3.14159265f / 180, dx = std::sin(a), dy = -std::cos(a);
    float len = std::fabs(box.w * dx) + std::fabs(box.h * dy); if (len < 1e-3f) return c0;
    Vec2 c = box.center(); float t = ((x - c.x) * dx + (y - c.y) * dy) / len + .5f;
    return Color::mix(c0, c1, std::clamp(t, 0.f, 1.f));
  }
};

// ============================================================================================
// Path : sous-chemins aplatis (polylignes) ; parseur de l'attribut SVG « d »
// ============================================================================================
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
    if ((*p == 'e' || *p == 'E') && (p[1] == '-' || p[1] == '+' || (p[1] >= '0' && p[1] <= '9'))) { p += 2; while (*p >= '0' && *p <= '9') ++p; }
    if (p == b) return false; out = std::strtof(b, nullptr); return true;
  }
  static bool flag(const char*& p, bool& out) {
    while (*p == ' ' || *p == ',') ++p; if (*p != '0' && *p != '1') return false; out = *p++ == '1'; return true;
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

// ============================================================================================
// Painter : primitives 2D + pile de transformations (translation, échelle uniforme, opacité, clip)
// ============================================================================================
class Painter {
 public:
  explicit Painter(Canvas& c) : cv(c) { st.clip = {0, 0, float(c.w), float(c.h)}; }
  struct State { Vec2 o{0, 0}; float s = 1, op = 1; Rect clip; };
  void push() { stack.push_back(st); }
  void pop() { st = stack.back(); stack.pop_back(); }
  void translate(float x, float y) { st.o.x += x * st.s; st.o.y += y * st.s; }
  void scaleAbout(Vec2 c, float k) {  // échelle autour d'un point (coordonnées locales)
    Vec2 d = map(c); st.s *= k; Vec2 d2 = map(c); st.o.x += d.x - d2.x; st.o.y += d.y - d2.y;
  }
  void opacity(float k) { st.op *= k; }
  void clip(Rect r) { Rect m = map(r); float x0 = std::max(st.clip.x, m.x), y0 = std::max(st.clip.y, m.y), x1 = std::min(st.clip.x + st.clip.w, m.x + m.w), y1 = std::min(st.clip.y + st.clip.h, m.y + m.h); st.clip = {x0, y0, std::max(0.f, x1 - x0), std::max(0.f, y1 - y0)}; }
  Vec2 map(Vec2 p) const { return {st.o.x + p.x * st.s, st.o.y + p.y * st.s}; }
  Rect map(Rect r) const { return {st.o.x + r.x * st.s, st.o.y + r.y * st.s, r.w * st.s, r.h * st.s}; }
  float scale() const { return st.s; }

  // ---- rectangles arrondis (SDF anti-crénelé) ----
  void fillRRect(Rect r, float rad, const Paint& p) {
    Rect m = map(r); rad *= st.s; forPx(m, 1, [&](int x, int y) { float cov = rrCov(m, rad, x + .5f, y + .5f); if (cov > 0) blend(x, y, p.at(m, x + .5f, y + .5f), cov); });
  }
  void strokeRRect(Rect r, float rad, float wd, const Paint& p) {  // trait INTÉRIEUR (comme border CSS)
    Rect m = map(r); rad *= st.s; wd *= st.s;
    forPx(m, 1, [&](int x, int y) { float c = rrCov(m, rad, x + .5f, y + .5f) - rrCov(m.inset(wd), std::max(0.f, rad - wd), x + .5f, y + .5f); if (c > 0) blend(x, y, p.at(m, x + .5f, y + .5f), c); });
  }
  // Ombre portée (blur CSS = 2σ), découpée hors de la forme comme box-shadow.
  void shadow(Rect r, float rad, float dy, float blur, Color c) {
    Rect m = map(r); rad *= st.s; dy *= st.s; blur *= st.s; float pad = blur * 1.5f + 2;
    int x0 = int(m.x - pad), y0 = int(m.y + dy - pad), W = int(m.w + 2 * pad) + 1, H = int(m.h + 2 * pad) + 1;
    std::vector<float> a(size_t(W) * H);
    Rect sh{m.x, m.y + dy, m.w, m.h};
    for (int j = 0; j < H; ++j) for (int i = 0; i < W; ++i) a[size_t(j) * W + i] = rrCov(sh, rad, x0 + i + .5f, y0 + j + .5f);
    boxBlur(a, W, H, 1, blur / 2);
    for (int j = 0; j < H; ++j) for (int i = 0; i < W; ++i) {
      int x = x0 + i, y = y0 + j; float out = 1 - rrCov(m, rad, x + .5f, y + .5f);
      if (out > 0) blend(x, y, c, a[size_t(j) * W + i] * out);
    }
  }
  // ★ VERRE : floute et sature ce qui est DÉJÀ dessiné derrière r, puis teinte + bordure + reflet haut.
  void glass(Rect r, float rad, const Theme& th, Color tint, Color line) {
    Rect m = map(r); float R = rad * st.s; int x0 = std::max(0, int(std::floor(m.x))), y0 = std::max(0, int(std::floor(m.y)));
    int x1 = std::min(cv.w, int(std::ceil(m.x + m.w))), y1 = std::min(cv.h, int(std::ceil(m.y + m.h))); int W = x1 - x0, H = y1 - y0; if (W <= 0 || H <= 0) return;
    float sg = th.blur * st.s / 2;  // CSS blur(36px) → écart-type 18
    std::vector<float> ch[3]; for (auto& v : ch) v.resize(size_t(W) * H);
    for (int j = 0; j < H; ++j) for (int i = 0; i < W; ++i) { float* p = cv.at(x0 + i, y0 + j); for (int k = 0; k < 3; ++k) ch[k][size_t(j) * W + i] = p[k]; }
    for (auto& v : ch) boxBlur(v, W, H, 1, sg);
    float s = th.sat;  // saturate(): matrice CSS
    float M[9] = {.213f + .787f * s, .715f - .715f * s, .072f - .072f * s, .213f - .213f * s, .715f + .285f * s, .072f - .072f * s, .213f - .213f * s, .715f - .715f * s, .072f + .928f * s};
    for (int j = 0; j < H; ++j) for (int i = 0; i < W; ++i) {
      float cov = rrCov(m, R, x0 + i + .5f, y0 + j + .5f) * clipIn(x0 + i, y0 + j); if (cov <= 0) continue;
      size_t o = size_t(j) * W + i; float r0 = ch[0][o], g0 = ch[1][o], b0 = ch[2][o];
      Color c{std::clamp(M[0] * r0 + M[1] * g0 + M[2] * b0, 0.f, 1.f), std::clamp(M[3] * r0 + M[4] * g0 + M[5] * b0, 0.f, 1.f), std::clamp(M[6] * r0 + M[7] * g0 + M[8] * b0, 0.f, 1.f), st.op};
      float* d = cv.at(x0 + i, y0 + j); float k = cov * st.op;  // remplace l'arrière-plan par sa version floutée
      d[0] += (c.r - d[0]) * k; d[1] += (c.g - d[1]) * k; d[2] += (c.b - d[2]) * k;
    }
    fillRRect(r, rad, tint);
    strokeRRect(r, rad, 1, line);
    // reflet interne haut : inset 0 1px 0 rgba(255,255,255,.75)
    Rect hi = r; hi.y += 1; hi.h = std::min(r.h, 14.f);
    strokeRRectGradTop(r, rad, th.dark ? .20f : .75f);
  }
  // ---- vectoriel ----
  void fillPath(const Path& p, Vec2 pos, float sc, const Paint& paint, Rect box = {}) {
    float S = st.s * sc; Vec2 O = map(pos);
    float miny = 1e9f, maxy = -1e9f, minx = 1e9f, maxx = -1e9f;
    for (auto& sub : p.subs) for (auto& q : sub.pts) { miny = std::min(miny, O.y + q.y * S); maxy = std::max(maxy, O.y + q.y * S); minx = std::min(minx, O.x + q.x * S); maxx = std::max(maxx, O.x + q.x * S); }
    if (box.w == 0) box = {minx, miny, maxx - minx, maxy - miny};
    const int SS = 4; int W = cv.w; std::vector<float> row(W);
    struct X { float x; int d; }; std::vector<X> xs;
    for (int y = std::max(0, int(miny)); y <= std::min(cv.h - 1, int(maxy)); ++y) {
      std::fill(row.begin(), row.end(), 0.f);
      for (int k = 0; k < SS; ++k) {
        float sy = y + (k + .5f) / SS; xs.clear();
        for (auto& sub : p.subs) { size_t n = sub.pts.size(); if (n < 2) continue;
          for (size_t i = 0; i < n; ++i) { Vec2 a = sub.pts[i], b = sub.pts[(i + 1) % n];
            float ay = O.y + a.y * S, by = O.y + b.y * S; if (ay == by) continue;
            if ((sy >= ay) != (sy >= by)) { float t = (sy - ay) / (by - ay); xs.push_back({O.x + a.x * S + (b.x - a.x) * S * t, by > ay ? 1 : -1}); } } }
        std::sort(xs.begin(), xs.end(), [](const X& a, const X& b) { return a.x < b.x; });
        int wind = 0; for (size_t i = 0; i + 1 < xs.size(); ++i) { wind += xs[i].d; if (wind != 0) {
          float xa = std::max(0.f, xs[i].x), xb = std::min(float(W), xs[i + 1].x);
          for (int px = int(xa); px < xb && px < W; ++px) row[px] += std::max(0.f, std::min(xb, px + 1.f) - std::max(xa, float(px))) / SS; } }
      }
      for (int x = 0; x < W; ++x) if (row[x] > 0) blend(x, y, paint.at(box, x + .5f, y + .5f), std::min(1.f, row[x]));
    }
  }
  void strokePath(const Path& p, Vec2 pos, float sc, float width, const Paint& paint) {  // bouts et jointures arrondis
    float S = st.s * sc, hw = width * S / 2; Vec2 O = map(pos);
    struct Seg { Vec2 a, b; }; std::vector<Seg> segs; float minx = 1e9f, miny = 1e9f, maxx = -1e9f, maxy = -1e9f;
    for (auto& sub : p.subs) { size_t n = sub.pts.size(); size_t m = sub.closed ? n : n - 1; if (n == 1) { segs.push_back({sub.pts[0], sub.pts[0]}); }
      for (size_t i = 0; i < m && n > 1; ++i) segs.push_back({sub.pts[i], sub.pts[(i + 1) % n]}); }
    for (auto& s : segs) for (Vec2 q : {s.a, s.b}) { minx = std::min(minx, O.x + q.x * S); maxx = std::max(maxx, O.x + q.x * S); miny = std::min(miny, O.y + q.y * S); maxy = std::max(maxy, O.y + q.y * S); }
    Rect box{minx, miny, maxx - minx, maxy - miny};
    for (int y = int(miny - hw - 1); y <= int(maxy + hw + 1); ++y) for (int x = int(minx - hw - 1); x <= int(maxx + hw + 1); ++x) {
      float px = x + .5f, py = y + .5f, best = 1e9f;
      for (auto& s : segs) { float ax = O.x + s.a.x * S, ay = O.y + s.a.y * S, bx = O.x + s.b.x * S, by = O.y + s.b.y * S, dx = bx - ax, dy = by - ay, L = dx * dx + dy * dy;
        float t = L > 0 ? std::clamp(((px - ax) * dx + (py - ay) * dy) / L, 0.f, 1.f) : 0; float ex = ax + dx * t - px, ey = ay + dy * t - py; best = std::min(best, ex * ex + ey * ey); }
      float cov = std::clamp(hw - std::sqrt(best) + .5f, 0.f, 1.f); if (cov > 0) blend(x, y, paint.at(box, px, py), cov);
    }
  }
  // Icône 24×24 de baura-icons.js (trait 1.9, extrémités arrondies) dessinée dans r, couleur c.
  void icon(const char* name, Rect r, Color c, float strokeW = 1.9f) {
    for (size_t i = 0; i < icons::COUNT; ++i) if (!std::strcmp(icons::ALL[i].name, name)) {
      float sc = std::min(r.w, r.h) / 24.f; Vec2 pos{r.x + (r.w - 24 * sc) / 2, r.y + (r.h - 24 * sc) / 2};
      for (size_t k = 0; k < icons::ALL[i].n; ++k) { Path p = Path::fromSVG(icons::ALL[i].elems[k].d, 16);
        if (icons::ALL[i].elems[k].fill) fillPath(p, pos, sc, c); strokePath(p, pos, sc, strokeW, c); }
      return; }
  }
  // Symbole BAURA : viewBox 150 29 199 420 (barre + 2 demi-disques + point doré)
  void logo(Rect r, Color c1, Color c2, Color c3, Color c4) {
    float sc = std::min(r.w / 199.f, r.h / 420.f); Vec2 pos{r.x + (r.w - 199 * sc) / 2, r.y + (r.h - 420 * sc) / 2};  // viewBox d'origine (150,29) retiré ci-dessous
    pos.x -= 150 * sc; pos.y -= 29 * sc;
    Path bar = Path::fromSVG("M174 29A24 24 0 0 1 198 53V425A24 24 0 0 1 150 425V53A24 24 0 0 1 174 29Z", 24);
    Path top = Path::fromSVG("M248 29A101 102.5 0 0 1 248 234Z", 64), bot = Path::fromSVG("M248 245A101 102 0 0 1 248 449Z", 64);
    Path dot = Path::fromSVG("M331 221H334A15 15 0 0 1 349 236V242A15 15 0 0 1 334 257H331A15 15 0 0 1 316 242V236A15 15 0 0 1 331 221Z", 16);
    fillPath(bar, pos, sc, c1); fillPath(top, pos, sc, c2); fillPath(bot, pos, sc, c3); fillPath(dot, pos, sc, c4);
  }
  Canvas& canvas() { return cv; }

 private:
  Canvas& cv; State st; std::vector<State> stack;
  float clipIn(int x, int y) const { return (x < st.clip.x || y < st.clip.y || x >= st.clip.x + st.clip.w || y >= st.clip.y + st.clip.h) ? 0.f : 1.f; }
  static float sdRR(const Rect& r, float rad, float x, float y) {
    rad = std::min(rad, std::min(r.w, r.h) / 2); float cx = r.x + r.w / 2, cy = r.y + r.h / 2, qx = std::fabs(x - cx) - (r.w / 2 - rad), qy = std::fabs(y - cy) - (r.h / 2 - rad);
    return std::sqrt(std::pow(std::max(qx, 0.f), 2) + std::pow(std::max(qy, 0.f), 2)) + std::min(std::max(qx, qy), 0.f) - rad;
  }
  static float rrCov(const Rect& r, float rad, float x, float y) { return std::clamp(.5f - sdRR(r, rad, x, y), 0.f, 1.f); }
  template <class F> void forPx(const Rect& m, int pad, F f) {
    int x0 = std::max(int(std::floor(m.x)) - pad, int(st.clip.x)), y0 = std::max(int(std::floor(m.y)) - pad, int(st.clip.y));
    int x1 = std::min(int(std::ceil(m.x + m.w)) + pad, int(st.clip.x + st.clip.w)), y1 = std::min(int(std::ceil(m.y + m.h)) + pad, int(st.clip.y + st.clip.h));
    for (int y = std::max(0, y0); y < std::min(cv.h, y1); ++y) for (int x = std::max(0, x0); x < std::min(cv.w, x1); ++x) f(x, y);
  }
  void blend(int x, int y, Color c, float cov) {
    if (x < st.clip.x || y < st.clip.y || x >= st.clip.x + st.clip.w || y >= st.clip.y + st.clip.h) return;
    c.a *= st.op; cv.over(x, y, c, cov);
  }
  void strokeRRectGradTop(Rect r, float rad, float a0) {  // liseré lumineux dégradé du haut vers le bas
    Paint p = Paint::linear(180, Color{1, 1, 1, a0}, Color{1, 1, 1, 0});
    Rect m = map(r); float R = rad * st.s; Rect box{m.x, m.y, m.w, std::min(m.h, 18.f * st.s)};
    forPx(Rect{m.x, m.y, m.w, std::min(m.h, 18.f * st.s)}, 0, [&](int x, int y) {
      float c = rrCov(m, R, x + .5f, y + .5f) - rrCov(Rect{m.x, m.y + st.s, m.w, m.h}, R, x + .5f, y + .5f); if (c > 0) blend(x, y, p.at(box, x + .5f, y + .5f), c); });
  }
  // flou gaussien ≈ 3 passes de box blur (rayon dérivé de σ)
  static void boxBlur(std::vector<float>& a, int W, int H, int passes, float sigma) {
    if (sigma < .5f) return;
    int n = 3; float wi = std::sqrt(12 * sigma * sigma / n + 1); int wl = int(std::floor(wi)); if (wl % 2 == 0) --wl; int wu = wl + 2;
    float mi = (12 * sigma * sigma - n * wl * wl - 4 * n * wl - 3 * n) / (-4 * wl - 4); int m = int(std::round(mi));
    std::vector<float> t(a.size());
    for (int p = 0; p < passes * n; ++p) {
      int r = ((p % n) < m ? wl : wu) / 2;
      for (int y = 0; y < H; ++y) { float acc = 0; for (int x = -r; x <= r; ++x) acc += a[size_t(y) * W + std::clamp(x, 0, W - 1)];
        for (int x = 0; x < W; ++x) { t[size_t(y) * W + x] = acc / (2 * r + 1); acc += a[size_t(y) * W + std::min(W - 1, x + r + 1)] - a[size_t(y) * W + std::max(0, x - r)]; } }
      for (int x = 0; x < W; ++x) { float acc = 0; for (int y = -r; y <= r; ++y) acc += t[size_t(std::clamp(y, 0, H - 1)) * W + x];
        for (int y = 0; y < H; ++y) { a[size_t(y) * W + x] = acc / (2 * r + 1); acc += t[size_t(std::min(H - 1, y + r + 1)) * W + x] - t[size_t(std::max(0, y - r)) * W + x]; } }
    }
  }
};

}  // namespace bge
