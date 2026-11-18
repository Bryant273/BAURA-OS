// Outil de test : écrit un framebuffer ARGB en PPM (converti en PNG par les scripts de doc).
#pragma once
#include <cstdio>
#include <cstdint>
#include <vector>
inline bool write_ppm(const char* file, const std::vector<uint32_t>& px, int w, int h) {
  FILE* f = std::fopen(file, "wb"); if (!f) return false; std::fprintf(f, "P6\n%d %d\n255\n", w, h);
  for (int i = 0; i < w * h; ++i) { std::fputc((px[i] >> 16) & 255, f); std::fputc((px[i] >> 8) & 255, f); std::fputc(px[i] & 255, f); }
  std::fclose(f); return true;
}
inline std::vector<uint8_t> read_file(const char* path) { std::vector<uint8_t> d; FILE* f = std::fopen(path, "rb"); if (!f) return d; std::fseek(f, 0, SEEK_END); long n = std::ftell(f); std::fseek(f, 0, SEEK_SET); d.resize(size_t(n)); if (std::fread(d.data(), 1, d.size(), f) != d.size()) d.clear(); std::fclose(f); return d; }
