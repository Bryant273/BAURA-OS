#ifndef BAURA_BGE_RENDERER_CORE_H
#define BAURA_BGE_RENDERER_CORE_H

#include "renderer.h"

bool bge_framebuffer_valid(const BGEFramebuffer &framebuffer);
uint32_t bge_blend_pixel(uint32_t background, uint32_t foreground,
                         uint8_t alpha);

#endif
