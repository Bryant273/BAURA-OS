// BGE – backend plateforme SDL2 (fenêtre + entrées + affichage de la Canvas).
// ⚠ Écrit pour la doc ; NON compilé dans ce prototype (SDL2 absent de l'environnement de test).
// Build : g++ -std=c++17 -Iinclude src/platform_sdl2.cpp -lSDL2 -o bge_sdl
#include <SDL2/SDL.h>
#include "bge/components.hpp"
using namespace bge;

int main() {
  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window* win = SDL_CreateWindow("BAURA OS", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1440, 900, SDL_WINDOW_SHOWN);
  SDL_Renderer* rd = SDL_CreateRenderer(win, -1, SDL_RENDERER_PRESENTVSYNC);
  SDL_Texture* tex = SDL_CreateTexture(rd, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 1440, 900);
  PlaceholderText txt; Engine eng(1440, 900, txt);
  // … construire l'arbre comme dans examples/desktop_demo.cpp …
  std::vector<uint8_t> rgb(1440 * 900 * 3); Uint32 last = SDL_GetTicks(); bool run = true;
  while (run) {
    for (SDL_Event e; SDL_PollEvent(&e);) {
      if (e.type == SDL_QUIT) run = false;
      if (e.type == SDL_MOUSEMOTION) eng.dispatch({Event::Move, {float(e.motion.x), float(e.motion.y)}});
      if (e.type == SDL_MOUSEBUTTONDOWN) eng.dispatch({Event::Down, {float(e.button.x), float(e.button.y)}, e.button.button});
      if (e.type == SDL_MOUSEBUTTONUP) eng.dispatch({Event::Up, {float(e.button.x), float(e.button.y)}, e.button.button});
    }
    Uint32 now = SDL_GetTicks(); float dt = (now - last) / 1000.f; last = now;
    Canvas& cv = eng.frame(dt, [&](Painter& p) { p.fillRRect({0, 0, 1440, 900}, 0, Paint::linear(150, Color::hex(0xF1E4FF), Color::hex(0xA99BFF))); });
    for (size_t i = 0; i < size_t(cv.w) * cv.h; ++i) for (int c = 0; c < 3; ++c) rgb[i * 3 + c] = uint8_t(std::clamp(cv.px[i * 4 + c] + (1 - cv.px[i * 4 + 3]), 0.f, 1.f) * 255);
    SDL_UpdateTexture(tex, nullptr, rgb.data(), 1440 * 3); SDL_RenderCopy(rd, tex, nullptr, nullptr); SDL_RenderPresent(rd);
  }
  SDL_Quit();
}
