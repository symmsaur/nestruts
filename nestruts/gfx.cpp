#include "gfx.h"
#include "log.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>

graphics::graphics() {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    window =
        SDL_CreateWindow("Nestruts", SDL_WINDOWPOS_UNDEFINED,
                         SDL_WINDOWPOS_UNDEFINED, 1400, 1000, SDL_WINDOW_SHOWN);
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
void graphics::draw_pixel(int nes_x, int nes_y, rgb color) {
    uint32_t color_argb = (color.red << 16) + (color.green << 8) + color.blue;
    auto const surf = SDL_GetWindowSurface(window);
    SDL_LockSurface(surf);
    int fatness = 4;
    if (current_log_level == log_level::debug)
        fatness = 2;
    auto const pixels = static_cast<uint32_t *>(surf->pixels);
    auto const width = surf->pitch / sizeof(uint32_t);
    for (int x = nes_x * fatness; x < nes_x * fatness + fatness; x++) {
        for (int y = nes_y * fatness; y < nes_y * fatness + fatness; y++) {
            if (x >= surf->w || y >= surf->h) continue;
            *(pixels + x + y * width) = color_argb;
        }
    }
    SDL_UnlockSurface(surf);
}
