// BAURA OS — baura_glass.h : verre fidèle à la charte (flou gaussien + saturation + teinte + bordure + reflet) et ombres portées.
// Remplace/complète GUI/BGE/effects_engine.* (flou « boîte » à 1 passe, rayon ≤ 32, sans saturation ni ombre).
#ifndef BAURA_GLASS_H
#define BAURA_GLASS_H
#include "baura_vector.h"

namespace BauraVec {
struct GlassStyle {
  float blur = 36;       // « --blur » : σ = blur/2
  float saturate = 1.9f; // « --sat »
  uint32_t tint = 0x57FFFFFF;   // --glass (clair) : rgba(255,255,255,.34)
  uint32_t line = 0xB3FFFFFF;   // --glass-line
  float highlight = .75f;       // reflet haut (clair .75 / sombre .20)
  bool opaque_fallback = false; // « transparence réduite » : pas de flou, surface pleine
};

// Flou gaussien ≈ 3 passes de flou « boîte » (largeurs dérivées de σ) — coût linéaire en pixels, indépendant de σ.
inline void box_blur_f(std::vector<float>& a, int W, int H, float sigma) {
  if (sigma < .5f) return;
  const int n = 3;
  float wi = std::sqrt(12 * sigma * sigma / n + 1); int wl = int(std::floor(wi)); if (wl % 2 == 0) --wl; int wu = wl + 2;
  int m = int(std::round((12 * sigma * sigma - n * wl * wl - 4 * n * wl - 3 * n) / (-4.f * wl - 4))); std::vector<float> t(a.size());
  for (int p = 0; p < n; ++p) { int r = ((p < m) ? wl : wu) / 2; float inv = 1.f / (2 * r + 1);
    for (int y = 0; y < H; ++y) { float acc = 0; for (int x = -r; x <= r; ++x) acc += a[size_t(y) * W + std::clamp(x, 0, W - 1)];
      for (int x = 0; x < W; ++x) { t[size_t(y) * W + x] = acc * inv; acc += a[size_t(y) * W + std::min(W - 1, x + r + 1)] - a[size_t(y) * W + std::max(0, x - r)]; } }
    for (int x = 0; x < W; ++x) { float acc = 0; for (int y = -r; y <= r; ++y) acc += t[size_t(std::clamp(y, 0, H - 1)) * W + x];
      for (int y = 0; y < H; ++y) { a[size_t(y) * W + x] = acc * inv; acc += t[size_t(std::min(H - 1, y + r + 1)) * W + x] - t[size_t(std::max(0, y - r)) * W + x]; } } }
}

// Ombre portée découpée hors de la forme (comme box-shadow). dy = décalage vertical, blur = rayon CSS (σ = blur/2).
inline void shadow(Painter& p, Rect r, float rad, float dy, float blur, uint32_t color) {
  float pad = blur * 1.5f + 2; int x0 = int(r.x - pad), y0 = int(r.y + dy - pad), W = int(r.w + 2 * pad) + 1, H = int(r.h + 2 * pad) + 1;
  std::vector<float> a(size_t(W) * H); Rect sh{r.x, r.y + dy, r.w, r.h};
  for (int j = 0; j < H; ++j) for (int i = 0; i < W; ++i) a[size_t(j) * W + i] = Painter::cov_rr(sh, rad, x0 + i + .5f, y0 + j + .5f);
  box_blur_f(a, W, H, blur / 2);
  for (int j = 0; j < H; ++j) for (int i = 0; i < W; ++i) { float out = 1 - Painter::cov_rr(r, rad, x0 + i + .5f, y0 + j + .5f); if (out > 0) p.put(x0 + i, y0 + j, color, a[size_t(j) * W + i] * out); }
}

// Surface de verre : à appeler APRÈS avoir dessiné tout ce qui doit apparaître derrière (ordre Z).
inline void glass(Painter& p, Rect r, float rad, const GlassStyle& g) {
  BGERenderer& rd = p.renderer(); const BGEFramebuffer& fb = rd.framebuffer();
  int x0 = std::max(0, int(std::floor(r.x))), y0 = std::max(0, int(std::floor(r.y))), x1 = std::min(int(fb.width), int(std::ceil(r.x + r.w))), y1 = std::min(int(fb.height), int(std::ceil(r.y + r.h)));
  int W = x1 - x0, H = y1 - y0; if (W <= 0 || H <= 0) return;
  if (!g.opaque_fallback) {
    std::vector<float> ch[3]; for (auto& v : ch) v.resize(size_t(W) * H);
    for (int j = 0; j < H; ++j) for (int i = 0; i < W; ++i) { uint32_t c = rd.pixel(x0 + i, y0 + j); size_t o = size_t(j) * W + i; ch[0][o] = (c >> 16 & 255) / 255.f; ch[1][o] = (c >> 8 & 255) / 255.f; ch[2][o] = (c & 255) / 255.f; }
    for (auto& v : ch) box_blur_f(v, W, H, g.blur / 2);
    float s = g.saturate, M[9] = {.213f + .787f * s, .715f - .715f * s, .072f - .072f * s, .213f - .213f * s, .715f + .285f * s, .072f - .072f * s, .213f - .213f * s, .715f - .715f * s, .072f + .928f * s};
    for (int j = 0; j < H; ++j) for (int i = 0; i < W; ++i) { float cov = Painter::cov_rr(r, rad, x0 + i + .5f, y0 + j + .5f); if (cov <= 0) continue; size_t o = size_t(j) * W + i;
      float R = std::clamp(M[0] * ch[0][o] + M[1] * ch[1][o] + M[2] * ch[2][o], 0.f, 1.f), G = std::clamp(M[3] * ch[0][o] + M[4] * ch[1][o] + M[5] * ch[2][o], 0.f, 1.f), B = std::clamp(M[6] * ch[0][o] + M[7] * ch[1][o] + M[8] * ch[2][o], 0.f, 1.f);
      p.put(x0 + i, y0 + j, argb(255, int(R * 255 + .5f), int(G * 255 + .5f), int(B * 255 + .5f)), cov); }
  }
  p.fill_rrect(r, rad, g.tint); p.stroke_rrect(r, rad, 1, g.line);
  // reflet interne haut : liseré blanc dégradé sur 18 px
  Rect box{r.x, r.y, r.w, std::min(r.h, 18.f)}; Paint hi = Paint::linear(180, argb(int(255 * g.highlight), 255, 255, 255), argb(0, 255, 255, 255));
  for (int y = int(r.y); y < int(r.y + box.h); ++y) for (int x = int(r.x); x < int(r.x + r.w); ++x) { float c = Painter::cov_rr(r, rad, x + .5f, y + .5f) - Painter::cov_rr(Rect{r.x, r.y + 1, r.w, r.h}, rad, x + .5f, y + .5f); if (c > 0) p.put(x, y, hi.at(box, x + .5f, y + .5f), c); }
}
}  // namespace BauraVec
#endif
