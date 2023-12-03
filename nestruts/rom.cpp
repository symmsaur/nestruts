#include "rom.h"

#include <cstdio>

void load_rom(std::string const& filename, picture_processing_unit& ppu,
              memory_bus& bus) {
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
        bus.load_rom(static_cast<uint16_t>(i), val8);
    }
    // Load CHR ROM
    if (chr_rom_banks != 1)
        throw std::runtime_error(
            "Not implemented reading multiple CHR ROM banks.");
    for (int i = 0; i < 0x2000; i++) {
        int value = fgetc(rom);
        if (value < 0)
            throw std::runtime_error("Failed reading file.");
        ppu.load_rom(static_cast<uint16_t>(i), static_cast<uint8_t>(value));
    }
    log(log_level::info, "Finished loading\n");
    fclose(rom);
}
