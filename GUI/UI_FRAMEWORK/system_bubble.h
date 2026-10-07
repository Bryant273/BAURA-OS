#ifndef BAURA_SYSTEM_BUBBLE_H
#define BAURA_SYSTEM_BUBBLE_H

#include "../BGE/renderer.h"

#include <string>

class BauraSystemBubble {
public:
    BauraSystemBubble();
    void update_progress(const std::string &label, float value);
    void set_visible(bool visible);
    void render(BGERenderer &renderer, int x, int y, int width = 320);

private:
    std::string label;
    float progress;
    bool visible;
};

#endif
