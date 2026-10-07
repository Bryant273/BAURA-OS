#ifndef BAURA_BGE_GPU_ACCEL_3D_H
#define BAURA_BGE_GPU_ACCEL_3D_H

#include "renderer.h"

#include <vector>

struct BGEVertex {
    float x;
    float y;
    float depth;
    uint32_t color;
};

struct BGETriangle {
    BGEVertex first;
    BGEVertex second;
    BGEVertex third;
};

class BGE3DRenderer {
public:
    BGE3DRenderer(BGERenderer &renderer, uint32_t width, uint32_t height);
    bool valid() const;
    void clear_depth(float depth = 1.0f);
    bool draw_triangle(const BGETriangle &triangle);

private:
    BGERenderer &renderer;
    uint32_t width;
    uint32_t height;
    std::vector<float> depth_buffer;
};

#endif
