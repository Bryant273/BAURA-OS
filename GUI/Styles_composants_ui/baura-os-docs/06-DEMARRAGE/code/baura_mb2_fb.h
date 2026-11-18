// BAURA OS — baura_mb2_fb.h : lecture de l'étiquette framebuffer Multiboot2 (type 8) + conversion/présentation.
// Freestanding : aucune allocation, aucune exception, pas de libc (seulement <stdint.h>/<stddef.h>).
// Spécification : Multiboot2 §3.6.9 « Framebuffer info ».  À compiler dans BOOT/SEL4 (initial_task) ou GUI/PLATFORM.
#ifndef BAURA_MB2_FB_H
#define BAURA_MB2_FB_H
#include <stdint.h>
#include <stddef.h>
namespace BauraBoot {

struct FbInfo {                 // ce dont BauraMappedFramebuffer a besoin
  uint64_t phys_addr = 0;       // adresse PHYSIQUE (à mapper : voir README, section seL4)
  uint32_t pitch_bytes = 0, width = 0, height = 0;
  uint8_t  bpp = 0;             // 32 attendu
  uint8_t  red_pos = 16, green_pos = 8, blue_pos = 0;   // position du bit de poids faible de chaque canal
  bool     rgb_direct = false;  // type 1 = « direct RGB » ; 0 = palette indexée, 2 = texte EGA : NON supportés
  bool valid() const { return rgb_direct && bpp == 32 && width >= 640 && height >= 480 && pitch_bytes >= width * 4u && phys_addr != 0; }
  size_t bytes() const { return size_t(pitch_bytes) * height; }
  // Correspond à BauraFramebufferFormat du dépôt : rouge en bits 16.. => octets en mémoire B,G,R,X (little-endian) = BGRX8888.
  bool is_bgrx() const { return red_pos == 16 && green_pos == 8 && blue_pos == 0; }
  bool is_rgbx() const { return red_pos == 0 && green_pos == 8 && blue_pos == 16; }
};

// Lecture non alignée little-endian (le blob Multiboot2 est aligné sur 8 octets, mais on ne le suppose pas).
inline uint32_t rd32(const uint8_t* p) { return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24; }
inline uint64_t rd64(const uint8_t* p) { return uint64_t(rd32(p)) | uint64_t(rd32(p + 4)) << 32; }

// `info` : adresse de la structure d'information Multiboot2 (EBX au démarrage, ou argument de l'amorce). `max` : borne de sécurité.
// Retourne false si l'étiquette est absente, tronquée ou si le mode n'est pas du RGB direct 32 bits.
inline bool find_framebuffer(const uint8_t* info, size_t max, FbInfo& out) {
  if (!info || max < 16) return false;
  uint32_t total = rd32(info); if (total < 16 || total > max) return false;
  size_t off = 8;
  while (off + 8 <= total) {
    uint32_t type = rd32(info + off), size = rd32(info + off + 4);
    if (type == 0 && size == 8) return false;                       // fin de liste
    if (size < 8 || off + size > total) return false;               // étiquette corrompue
    if (type == 8) {                                                // FRAMEBUFFER
      if (size < 31) return false;
      const uint8_t* t = info + off + 8; FbInfo f;
      f.phys_addr = rd64(t); f.pitch_bytes = rd32(t + 8); f.width = rd32(t + 12); f.height = rd32(t + 16); f.bpp = t[20];
      uint8_t fb_type = t[21];                                      // t[22..23] réservé
      f.rgb_direct = fb_type == 1;
      if (f.rgb_direct) {                                           // après : red_pos, red_size, green_pos, green_size, blue_pos, blue_size
        if (size < 32) return false;
        f.red_pos = t[24]; f.green_pos = t[26]; f.blue_pos = t[28];
        if (t[25] != 8 || t[27] != 8 || t[29] != 8) f.rgb_direct = false;   // seuls les canaux de 8 bits sont gérés
      }
      out = f; return f.valid();
    }
    off += (size + 7u) & ~7u;                                       // étiquettes alignées sur 8 octets
  }
  return false;
}

// Présente une image ARGB (0xAARRGGBB, `stride_px` pixels par ligne) dans un framebuffer à pitch propre.
// Même rôle que BauraGuiPlatform::present() : le pitch est en OCTETS et peut dépasser largeur×4.
inline void present_argb(volatile uint8_t* dst, const FbInfo& f, const uint32_t* src, uint32_t w, uint32_t h, uint32_t stride_px) {
  uint32_t cw = w < f.width ? w : f.width, ch = h < f.height ? h : f.height;
  for (uint32_t y = 0; y < ch; ++y) {
    volatile uint8_t* row = dst + size_t(y) * f.pitch_bytes; const uint32_t* s = src + size_t(y) * stride_px;
    if (f.is_bgrx()) { for (uint32_t x = 0; x < cw; ++x) { uint32_t c = s[x]; row[x*4+0] = uint8_t(c); row[x*4+1] = uint8_t(c >> 8); row[x*4+2] = uint8_t(c >> 16); row[x*4+3] = 0; } }
    else { for (uint32_t x = 0; x < cw; ++x) { uint32_t c = s[x]; row[x*4+0] = uint8_t(c >> 16); row[x*4+1] = uint8_t(c >> 8); row[x*4+2] = uint8_t(c); row[x*4+3] = 0; } }
  }
}
}  // namespace BauraBoot
#endif
