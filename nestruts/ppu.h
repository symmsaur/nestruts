#pragma once
#include <array>
#include <cstdint>

#include "gfx.h"

class picture_processing_unit {
  public:
    picture_processing_unit();
    void load_rom(uint16_t adr, uint8_t val);

    uint8_t read_PPUSTATUS();

    void set_PPUCTRL(uint8_t val);
    void set_PPUMASK(uint8_t val);
    void set_OAMADDR(uint8_t val);
    void write_PPUSCROLL(uint8_t val);
    void write_PPUADDR(uint8_t val);
    void write_PPUDATA(uint8_t val);

    void dma_write(uint8_t i, uint8_t val);

    // Set vblank status and return if NMI should fire.
    bool vblank();

    void draw_debug();

  private:
    void draw_tiles(uint16_t base_offset, int base_x, int base_y);
    void draw_tile(int base_x, int base_y, uint16_t tile_index,
                   int palette_number, bool flip_x = false, bool flip_y = false,
                   bool sprite = false);
    void draw_nametable();
    void draw_sprites();

    rgb palette_color(bool sprite, int palette_number, int val);
    // 8 K of CHR ROM
    std::array<uint8_t, 0x2000> rom{};
    // 2 K of RAM
    std::array<uint8_t, 0x0800> ram{};
    // 256 B of OAM
    std::array<uint8_t, 0x0100> oam{};
    // 32 B of palette data
    std::array<uint8_t, 0x0020> palette_data{};

    graphics gfx{};

    // registers
    uint8_t PPUCTRL = 0;
    uint8_t PPUMASK = 0;
    uint8_t OAMADDR = 0;
    uint8_t PPUSCROLL_X = 0;
    uint8_t PPUSCROLL_Y = 0;
    bool PPUSCROLL_latch = false;

    bool vblank_started = false;

    uint16_t PPUADDR = 0;
    bool PPUADDR_latch = false;
    uint8_t PPUDATA_buffer = 0;
};
