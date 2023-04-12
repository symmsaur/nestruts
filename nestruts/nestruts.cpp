// nestruts.cpp : Defines the entry point for the console application.
//

#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_scancode.h>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

#include "audio_processing_unit.h"
#include "controller.h"
#include "core6502.h"
#include "log.h"
#include "mem.h"

using namespace std::literals;

std::tuple<std::shared_ptr<picture_processing_unit>,
           std::unique_ptr<memory_bus>, std::shared_ptr<audio_processing_unit>,
           std::shared_ptr<controller>>
load_rom(std::string filename) {
    FILE *rom{std::fopen(filename.c_str(), "rb")};
    if (rom == nullptr) {
        throw std::runtime_error("Failed to open file '" + filename + "'.");
    }
    // read header
    auto const guard = std::array<uint8_t, 4>{'N', 'E', 'S', 0x1A};
    for (auto const c : guard) {
        auto value = fgetc(rom);
        if (value < 0)
            throw std::runtime_error("Failed to read file.");
        if (value != c)
            throw std::runtime_error("Unexpected rom file header guard.");
    }
    auto const prg_rom_banks = fgetc(rom);
    if (prg_rom_banks < 0)
        throw std::runtime_error("Failed to read file.");
    auto const chr_rom_banks = fgetc(rom);
    if (chr_rom_banks < 0)
        throw std::runtime_error("Failed to read file.");
    // Skip rest of header
    fseek(rom, 16, 0);
    // load PRG ROM
    auto ppu = std::make_shared<picture_processing_unit>();
    auto apu = std::make_shared<audio_processing_unit>();
    auto ctrl = std::make_shared<controller>();
    auto bus = std::make_unique<memory_bus>(ppu, apu, ctrl);
    for (int i = 0x8000; i <= 0xFFFF; i++) {
        if (prg_rom_banks == 1 && i == 0xC000) {
            // Mirror the single ROM bank twice.
            fseek(rom, 16, 0);
        }
        int value = fgetc(rom);
        if (value < 0)
            throw std::runtime_error("Failed reading file.");
        if (i % 0x1000 == 0)
            logf(log_level::debug, "loaded: %#06x\n", i);
        uint8_t val8 = (uint8_t)value;
        bus->load_rom(static_cast<uint16_t>(i), val8);
    }
    // Load CHR ROM
    if (chr_rom_banks != 1)
        throw std::runtime_error(
            "Not implemented reading multiple CHR ROM banks.");
    for (int i = 0; i < 0x2000; i++) {
        int value = fgetc(rom);
        if (value < 0)
            throw std::runtime_error("Failed reading file.");
        ppu->load_rom(static_cast<uint16_t>(i), static_cast<uint8_t>(value));
    }
    log(log_level::info, "Finished loading\n");
    fclose(rom);
    return {std::move(ppu), std::move(bus), std::move(apu), std::move(ctrl)};
}

int run_game(std::string const &rom_filename) {
    int status{0};
    auto [ppu, bus, apu, ctrl] =
        load_rom(rom_filename); // Hardcoded rom for now
    // The reset vector is always stored at this address in ROM.
    uint16_t const reset_vector = bus->read(0xFFFC) + (bus->read(0xFFFD) << 8);
    auto cpu = std::make_unique<core6502>(std::move(bus),
                                          [apu]() { return apu->IRQ(); });
    cpu->setpp(reset_vector);
    constexpr int cycles_per_frame{30000}; // Actual NTSC: 29780.5
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
        if (sleep_time_ms > 0) {
            SDL_Delay(sleep_time_ms);
        }
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
        ppu->draw_debug();
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
