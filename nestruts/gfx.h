#pragma once
#include <cstdint>

#include "SDL2/SDL.h"

// TODO: Refactor out rgb from here so we can do this nicer
#include "palette.h"

class graphics final {
  public:
    graphics();
    ~graphics();

    void clear();
    void flip();
    void draw();
    void draw_pixel(int x, int y, rgb color);

  private:
    SDL_Window *window = nullptr;
};
