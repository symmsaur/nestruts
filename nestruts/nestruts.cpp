// nestruts.cpp : Defines the entry point for the console application.
//

#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

#include "audio_processing_unit.h"
#include "core6502.h"
#include "log.h"
#include "mem.h"

std::tuple<std::shared_ptr<picture_processing_unit>,
           std::unique_ptr<memory_bus>, std::shared_ptr<audio_processing_unit>>
load_rom(std::string filename) {
    FILE *rom{std::fopen(filename.c_str(), "rb")};
    if (rom == nullptr) {
        throw std::runtime_error("failed to open file");
    }
    // read header
    auto const guard = std::array<uint8_t, 4>{'N', 'E', 'S', 0x1A};
    for (auto const c : guard) {
        auto value = fgetc(rom);
        if (value < 0)
            throw std::runtime_error("failed to read file");
        if (value != c)
            throw std::runtime_error("unexpected rom file header guard");
    }
    auto const prg_rom_banks = fgetc(rom);
    if (prg_rom_banks < 0)
        throw std::runtime_error("failed to read file");
    auto const chr_rom_banks = fgetc(rom);
    if (chr_rom_banks < 0)
        throw std::runtime_error("failed to read file");
    // Skip rest of header
    fseek(rom, 16, 0);
    // load PRG ROM
    auto ppu = std::make_shared<picture_processing_unit>();
    auto apu = std::make_shared<audio_processing_unit>();
    auto bus = std::make_unique<memory_bus>(ppu, apu);
    for (int i = 0x8000; i <= 0xFFFF; i++) {
        if (prg_rom_banks == 1 && i == 0xC000) {
            // Mirror the single ROM bank twice.
            fseek(rom, 16, 0);
        }
        int value = fgetc(rom);
        if (value < 0)
            throw std::runtime_error("failed reading file\n");
        if (i % 0x1000 == 0)
            printf("loaded: %#06x\n", i);
        uint8_t val8 = (uint8_t)value;
        bus->load_rom(static_cast<uint16_t>(i), val8);
    }
    // Load CHR ROM
    if (chr_rom_banks != 1)
        throw std::runtime_error("not implemented");
    for (int i = 0; i < 0x2000; i++) {
        int value = fgetc(rom);
        if (value < 0)
            throw std::runtime_error("failed reading file\n");
        ppu->load_rom(static_cast<uint16_t>(i), static_cast<uint8_t>(value));
    }
    printf("finished loading\n");
    fclose(rom);
    return {std::move(ppu), std::move(bus), std::move(apu)};
}

int test_game() {
    auto [ppu, bus, apu] = load_rom("roms/dk.nes"); // Hardcoded rom for now
    // The reset vector is always stored at this address in ROM.
    uint16_t reset_vector = bus->read(0xFFFC) + (bus->read(0xFFFD) << 8);
    auto cpu = std::make_unique<core6502>(
        std::move(bus), [ppu]() { return ppu->NMI(); },
        [apu]() { return apu->IRQ(); });
    cpu->setpp(reset_vector);
    current_log_level = log_level::info;
    uint64_t cycles{};
    while (true) {
        ++cycles;
        if (!cpu->is_faulted()) {
            cpu->cycle();
            ppu->cycle();
            apu->cycle();
            if (cycles % 100000 == 0) {
                log(log_level::info, ".");
            }
        }
        if (cycles % 1000 == 0) {
            SDL_Event e;
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_QUIT) {
                    goto exit;
                }
                if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_SPACE) {
                    current_log_level = log_level::debug;
                }
            }
        }
    }
exit:
    return 0;
}

int main(int argc, char *argv[]) {
    static_cast<void>(argc);
    static_cast<void>(argv);
    test_game();

    return 0;
}
