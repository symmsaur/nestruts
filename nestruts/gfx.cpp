#include "gfx.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>

graphics::graphics() {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    window =
        SDL_CreateWindow("Nestruts", SDL_WINDOWPOS_UNDEFINED,
                         SDL_WINDOWPOS_UNDEFINED, 1400, 900, SDL_WINDOW_SHOWN);
}

graphics::~graphics() { SDL_DestroyWindow(window); }

void graphics::clear() {
    auto surf = SDL_GetWindowSurface(window);
    SDL_LockSurface(surf);
    SDL_FillRect(surf, nullptr, 0);
    SDL_UnlockSurface(surf);
}

void graphics::flip() { SDL_UpdateWindowSurface(window); }

void graphics::draw() {}

// Draw a fat pixel
void graphics::draw_pixel(int nes_x, int nes_y, uint8_t color) {
    // Map the two bits to RG
    auto red = (color % 2) * 128;
    auto green = ((color >> 1) % 2) * 255;
    auto blue = color * 64;
    uint32_t color_argb =
        ((red + (color ? 127 : 0)) << 16) + (green << 8) + blue;
    auto surf = SDL_GetWindowSurface(window);
    SDL_LockSurface(surf);
    constexpr int fatness = 2;
    for (int x = nes_x * fatness; x < nes_x * fatness + fatness; x++) {
        for (int y = nes_y * fatness; y < nes_y * fatness + fatness; y++) {
            *(static_cast<uint32_t *>(surf->pixels) + x +
              y * surf->pitch / sizeof(uint32_t)) = color_argb;
        }
    }
    SDL_UnlockSurface(surf);
}
