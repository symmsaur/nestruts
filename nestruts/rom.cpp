#include "rom.h"

#include <cstdio>

// Helper class for managing FILE lifetime
class file {
  private:
    FILE* m_stream{};

  public:
    file(std::string const &filename)
        : m_stream{std::fopen(filename.c_str(), "rb")} {
        if (m_stream == nullptr) {
            throw std::runtime_error("Failed to open file '" + filename + "'.");
        }
    }
    file(file const &) = delete;
    file &operator=(file const &) = delete;
    file(file &&) = delete;
    file &operator=(file &&) = delete;
    ~file() { fclose(m_stream); }

    FILE *stream() { return m_stream; }
};

void load_rom(std::string const& filename, picture_processing_unit& ppu,
              memory_bus& bus) {
    file rom{filename};

    // read header
    auto const guard = std::array<uint8_t, 4>{'N', 'E', 'S', 0x1A};
    for (auto const c : guard) {
        int const value{fgetc(rom.stream())};
        if (value < 0)
            throw std::runtime_error("Failed to read file.");
        if (value != c)
            throw std::runtime_error("Unexpected rom file header guard.");
    }
    int const num_prg_rom_banks{fgetc(rom.stream())};
    if (num_prg_rom_banks < 0)
        throw std::runtime_error("Failed to read file.");
    int const num_chr_rom_banks{fgetc(rom.stream())};
    if (num_chr_rom_banks < 0)
        throw std::runtime_error("Failed to read file.");

    // Skip rest of header
    fseek(rom.stream(), 16, 0);

    // load PRG ROM
    for (int i{0x8000}; i < 0x10000; i++) {
        if (num_prg_rom_banks == 1 && i == 0xC000) {
            // Mirror the single ROM bank twice.
            fseek(rom.stream(), 16, 0);
        }
        int const value{fgetc(rom.stream())};
        if (value < 0)
            throw std::runtime_error("Failed reading file.");
        if (i % 0x1000 == 0)
            logf(log_level::debug, "loaded: %#06x\n", i);
        bus.load_rom(static_cast<uint16_t>(i), static_cast<uint8_t>(value));
    }

    // Load CHR ROM
    if (num_chr_rom_banks != 1)
        throw std::runtime_error(
            "Not implemented reading multiple CHR ROM banks.");
    for (int i{0}; i < 0x2000; i++) {
        int const value{fgetc(rom.stream())};
        if (value < 0)
            throw std::runtime_error("Failed reading file.");
        ppu.load_rom(static_cast<uint16_t>(i), static_cast<uint8_t>(value));
    }
    log(log_level::info, "Finished loading\n");
}
