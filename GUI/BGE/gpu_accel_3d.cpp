
#include <vector>
#include "renderer_core.cpp"

struct Vertex { float x, y, z; uint32_t color; };
struct Triangle { Vertex v1, v2, v3; };

class BGE3DRenderer {
    float* z_buffer;
    uint32_t screen_w, screen_h;

public:
    BGE3DRenderer(uint32_t w, uint32_t h) : screen_w(w), screen_h(h) {
        z_buffer = new float[w * h];
    }

    void clear_z() {
        for (uint32_t i = 0; i < screen_w * screen_h; i++) z_buffer[i] = 1.0f;
    }

    void draw_triangle(Triangle t) {
        // Standard Barycentric Rasterization
        // 1. Calculate bounding box
        // 2. For each pixel in box, compute depth Z
        // 3. If Z < z_buffer[pixel], update pixel and z_buffer
        // This allows for complex 3D UI elements (overlapping floating panels)
    }
};
