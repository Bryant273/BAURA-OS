// Test 06 : (1) rendu de la séquence de démarrage ; (2) analyse d'un blob Multiboot2 synthétique ; (3) présentation dans un framebuffer à pitch.
#include "baura_boot_splash.h"
#include "baura_mb2_fb.h"
#include "bmp_out.h"
#include <cstring>
#include <cstdio>
using namespace BauraUI; using namespace BauraBoot;
#define CHECK(c) do { if (!(c)) { std::printf("ECHEC ligne %d : %s\n", __LINE__, #c); return 1; } } while (0)
static void put32(std::vector<uint8_t>& b, uint32_t v) { for (int i = 0; i < 4; ++i) b.push_back(uint8_t(v >> (8 * i))); }
static void pad8(std::vector<uint8_t>& b) { while (b.size() % 8) b.push_back(0); }
static std::vector<uint8_t> blob(uint8_t rpos, uint8_t bpos, uint8_t type = 1, uint8_t bpp = 32) {
  std::vector<uint8_t> b; put32(b, 0); put32(b, 0);                       // total_size, reserved
  put32(b, 1); put32(b, 12); b.insert(b.end(), {'B','A','U','R'}); pad8(b);  // étiquette « ligne de commande » factice
  size_t s = b.size(); put32(b, 8); put32(b, 32);                          // framebuffer, size = 32
  put32(b, 0x00000000); put32(b, 0xFD);                                    // addr = 0xFD00000000 (grande adresse PCI)
  put32(b, 1280 * 4 + 64); put32(b, 1280); put32(b, 720); b.push_back(bpp); b.push_back(type); b.push_back(0); b.push_back(0);
  b.push_back(rpos); b.push_back(8); b.push_back(8); b.push_back(8); b.push_back(bpos); b.push_back(8); b.push_back(0); b.push_back(0);
  (void)s; pad8(b); put32(b, 0); put32(b, 8);
  uint32_t n = uint32_t(b.size()); for (int i = 0; i < 4; ++i) b[i] = uint8_t(n >> (8 * i)); return b;
}
int main() {
  auto fnt = read_file("../../common/inter.bgf"); Font font; font.load(fnt.data(), fnt.size());
  // ---------- 1. rendu : 6 instants, 640×360 ----------
  const int W = 640, H = 360; float ts[] = {.05f, .35f, .6f, .95f, 1.3f, 2.2f}; const int N = 6; const int PW = W / 2, PH = H / 2;
  std::vector<uint32_t> sheet(size_t(PW) * N * PH);
  for (int i = 0; i < N; ++i) {
    std::vector<uint32_t> px(size_t(W) * H); BGEFramebuffer fb{px.data(), W, H, W, px.size()}; BGERenderer r(fb); Painter p(r); Ui u{p, font, BauraBrand::DARK};
    draw_splash(u, W, H, ts[i], i < 4 ? 0.f : (i == 4 ? .35f : .8f), i < 4 ? "" : "Démarrage des services…");
    for (int y = 0; y < PH; ++y) for (int x = 0; x < PW; ++x) { uint32_t a = px[(y*2)*W + x*2], b = px[(y*2)*W + x*2+1], c = px[(y*2+1)*W + x*2], d = px[(y*2+1)*W + x*2+1];
      auto ch = [&](int s) { return ((a>>s&255)+(b>>s&255)+(c>>s&255)+(d>>s&255))/4; }; sheet[size_t(y) * PW * N + i * PW + x] = 0xFF000000 | ch(16) << 16 | ch(8) << 8 | ch(0); }
  }
  write_ppm("splash_strip.ppm", sheet, PW * N, PH);
  { std::vector<uint32_t> px(size_t(W) * H); BGEFramebuffer fb{px.data(), W, H, W, px.size()}; BGERenderer r(fb); Painter p(r); Ui u{p, font, BauraBrand::DARK}; draw_splash(u, W, H, 0, .8f, "Prêt", true); write_ppm("splash_final.ppm", px, W, H); }
  // ---------- 2. Multiboot2 ----------
  FbInfo f; auto b = blob(16, 0); CHECK(find_framebuffer(b.data(), b.size(), f));
  CHECK(f.phys_addr == 0xFD00000000ull && f.width == 1280 && f.height == 720 && f.pitch_bytes == 1280 * 4 + 64 && f.is_bgrx() && !f.is_rgbx());
  auto b2 = blob(0, 16); CHECK(find_framebuffer(b2.data(), b2.size(), f)); CHECK(f.is_rgbx());
  auto b3 = blob(16, 0, 0); CHECK(!find_framebuffer(b3.data(), b3.size(), f));         // palette indexée : refusé
  auto b4 = blob(16, 0, 2); CHECK(!find_framebuffer(b4.data(), b4.size(), f));         // texte EGA : refusé
  auto b5 = blob(16, 0, 1, 16); CHECK(!find_framebuffer(b5.data(), b5.size(), f));     // 16 bpp : refusé
  auto b6 = blob(16, 0); b6.resize(40); CHECK(!find_framebuffer(b6.data(), b6.size(), f));  // tronqué
  b6 = blob(16, 0); b6[8 + 16 + 4] = 3; CHECK(!find_framebuffer(b6.data(), b6.size(), f));  // taille d'étiquette corrompue
  CHECK(!find_framebuffer(nullptr, 0, f));
  // ---------- 3. présentation avec pitch ----------
  FbInfo g; g.phys_addr = 1; g.width = 640; g.height = 480; g.bpp = 32; g.rgb_direct = true; g.pitch_bytes = 640 * 4 + 64;
  std::vector<uint8_t> mem(g.bytes(), 0xEE); std::vector<uint32_t> src(640 * 480);
  for (int y = 0; y < 480; ++y) for (int x = 0; x < 640; ++x) src[size_t(y) * 640 + x] = 0xFF000000 | (x & 255) << 16 | (y & 255) << 8 | ((x ^ y) & 255);
  g.red_pos = 16; g.blue_pos = 0; present_argb(mem.data(), g, src.data(), 640, 480, 640);
  const uint8_t* q = &mem[size_t(7) * g.pitch_bytes + 9 * 4]; CHECK(q[0] == (9 ^ 7) && q[1] == 7 && q[2] == 9 && q[3] == 0);   // B,G,R,X
  CHECK(mem[size_t(7) * g.pitch_bytes + 640 * 4] == 0xEE);                          // l'octet de bourrage de fin de ligne n'est pas touché
  g.red_pos = 0; g.blue_pos = 16; present_argb(mem.data(), g, src.data(), 640, 480, 640); q = &mem[size_t(7) * g.pitch_bytes + 9 * 4]; CHECK(q[0] == 9 && q[2] == (9 ^ 7));  // R,G,B,X
  std::printf("06 : tous les tests passent\n"); return 0;
}
