#pragma once
#include <cstdint>

#include "SDL2/SDL.h"

class graphics final {
  public:
    graphics();
    ~graphics();

    void clear();
    void flip();
    void draw();
    void draw_pixel(int x, int y, uint8_t color);

  private:
    SDL_Window *window = nullptr;
};
