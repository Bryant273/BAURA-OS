#ifndef BAURA_UI_EXTENSIONS_H
#define BAURA_UI_EXTENSIONS_H

#include "../BGE/renderer.h"

#include <cstddef>
#include <cstdint>

void render_performance_graph(BGERenderer &renderer, int x, int y, int width,
                              int height, const float *data, size_t length,
                              uint32_t color);

#endif
