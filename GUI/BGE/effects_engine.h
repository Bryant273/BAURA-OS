#ifndef BAURA_BGE_EFFECTS_ENGINE_H
#define BAURA_BGE_EFFECTS_ENGINE_H

#include "renderer.h"

class BGEEffects {
public:
    explicit BGEEffects(BGERenderer &renderer);
    bool apply_blur(int x, int y, int width, int height, int radius);
    bool draw_glass_panel(int x, int y, int width, int height, int radius,
                          uint32_t tint, uint8_t alpha);

private:
    BGERenderer &renderer;
};

#endif
