// nestruts.cpp : Entry point
//

#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_scancode.h>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

#include "apu.h"
#include "controller.h"
#include "core6502.h"
#include "log.h"
#include "mem.h"
#include "rom.h"

using namespace std::literals;

std::tuple<std::shared_ptr<picture_processing_unit>,
           std::unique_ptr<memory_bus>, std::shared_ptr<audio_processing_unit>,
           std::shared_ptr<controller>>
start_system(std::string const& filename) {
    // load PRG ROM
    auto ppu = std::make_shared<picture_processing_unit>();
    auto apu = std::make_shared<audio_processing_unit>();
    auto ctrl = std::make_shared<controller>();
    auto bus = std::make_unique<memory_bus>(ppu, apu, ctrl);
    load_rom(filename, *ppu, *bus);
    return {std::move(ppu), std::move(bus), std::move(apu), std::move(ctrl)};
}

int run_game(std::string const &rom_filename) {
    int status{0};
    auto [ppu, bus, apu, ctrl] =
        start_system(rom_filename);
    // The reset vector is always stored at this address in ROM.
    uint16_t const reset_vector = bus->read(0xFFFC) + (bus->read(0xFFFD) << 8);
    auto cpu = std::make_unique<core6502>(std::move(bus),
                                          [apu]() { return apu->IRQ(); });
    cpu->setpp(reset_vector);
    // Actual NTSC: 29780.5. We run one instruction per cycle which is way
    // faster than a real 6502 so ~1/3 should still be plenty.
    constexpr int cycles_per_frame{10000};
    // Warm up for one frame (enough?)
    for (int i{0}; i < cycles_per_frame; ++i) {
        if (!cpu->is_faulted()) {
            cpu->cycle();
        } else {
            log(log_level::error, "CPU faulted:\n{}\n", cpu->dump_state());
            status = 1;
            break;
        }
    }
    int64_t const initial_frame_timestamp_ms = SDL_GetTicks64();
    log(log_level::info, "Warmup finished\n");
    for (int32_t frames{0};; ++frames) {
        // Aim for 60 frames per second
        int64_t sleep_time_ms = initial_frame_timestamp_ms +
                                frames * 1000 / 60 -
                                static_cast<int64_t>(SDL_GetTicks64());
        log(log_level::debug, "sleep time: {} ms\n", sleep_time_ms);
        if (sleep_time_ms > 0) {
            SDL_Delay(sleep_time_ms);
        }
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                goto exit;
            }
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_SPACE) {
                current_log_level = log_level::debug;
            }
        }
        ctrl->clear();
        int numkeys{};
        auto keyboard_state = SDL_GetKeyboardState(&numkeys);
        if (keyboard_state[SDL_SCANCODE_LEFT])
            ctrl->down(button::left);
        if (keyboard_state[SDL_SCANCODE_RIGHT])
            ctrl->down(button::right);
        if (keyboard_state[SDL_SCANCODE_UP])
            ctrl->down(button::up);
        if (keyboard_state[SDL_SCANCODE_DOWN])
            ctrl->down(button::down);
        if (keyboard_state[SDL_SCANCODE_Z])
            ctrl->down(button::b);
        if (keyboard_state[SDL_SCANCODE_X])
            ctrl->down(button::a);
        if (keyboard_state[SDL_SCANCODE_RETURN])
            ctrl->down(button::start);
        if (keyboard_state[SDL_SCANCODE_LSHIFT])
            ctrl->down(button::select);
        if (ppu->vblank()) {
            cpu->nmi();
        }

        for (int i{0}; i < cycles_per_frame && !cpu->is_faulted(); ++i) {
            cpu->cycle();
            if (i % 2 == 0)
                apu->cycle();
            if (cpu->is_faulted()) {
                log(log_level::error, "CPU faulted:\n{}\n", cpu->dump_state());
                status = 1;
                break;
            }
        }
        if (current_log_level == log_level::debug) {
            ppu->draw_debug();
        } else {
            ppu->draw();
        }
        apu->play_audio();
        if (cpu->is_faulted())
            break;
    }
exit:
    return status;
}

void print_usage() { std::cout << "Usage:\n\tnestruts [-d] FILENAME\n"; }

int main(int argc, char *argv[]) {
    current_log_level = log_level::info;
    int consumed_args{0};
    if (argc == 3 && "-d"sv == argv[1]) {
        current_log_level = log_level::debug;
        consumed_args++;
    }
    if (argc != consumed_args + 2) {
        log(log_level::error, "Unexpected number of command line arguments.\n");
        print_usage();
        return 1;
    }
    try {
        return run_game(argv[consumed_args + 1]);
    } catch (std::runtime_error const &error) {
        log(log_level::error, "Failed to run: {}\n", error.what());
        print_usage();
    }
}
