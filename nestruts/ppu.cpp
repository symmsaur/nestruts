#include "ppu.h"

#include <cassert>
#include <cmath>
#include <cstdint>

#include "log.h"
#include "palette.h"

picture_processing_unit::picture_processing_unit() = default;

void picture_processing_unit::load_rom(uint16_t adr, uint8_t val) {
    rom.at(adr) = val;
}

void picture_processing_unit::draw_tiles(uint16_t base_tile_index, int base_x,
                                         int base_y) {
    for (auto i = 0; i < 16; i++) {
        for (auto j = 0; j < 16; j++) {
            auto x = i * 9 + base_x;
            auto y = j * 9 + base_y;
            draw_tile(x, y, (j * 16 + i) + base_tile_index, 0);
        }
    }
}

rgb picture_processing_unit::palette_color(bool sprite, int palette_number,
                                           int val) {
    std::size_t index{0};
    if (sprite)
        index |= (1 << 4);
    index |= palette_number << 2;
    index |= val;
    log(log_level::trace, "sprite: {}, palette_number:, val: {}, index: {}\n",
        sprite, palette_number, val, index);
    return palette[palette_data[index]];
}

void picture_processing_unit::draw_tile(int base_x, int base_y,
                                        uint16_t tile_index, int palette_number,
                                        bool flip_x, bool flip_y, bool sprite) {
    auto const offset = tile_index * 16;
    auto const flip_func = [](auto const flip, auto const x) {
        if (flip)
            return 7 - x;
        return x;
    };
    for (auto y = 0; y < 8; y++) {
        // Two bit planes for the tile
        uint8_t low_bits = rom.at(offset + y);
        uint8_t high_bits = rom.at(offset + y + 8);
        for (auto x = 0; x < 8; x++) {
            // Take out one high and one low bit from the left.
            auto const val = (low_bits >> 7) + (high_bits >> 7) * 2;
            low_bits <<= 1;
            high_bits <<= 1;
            if (val != 0x00) {
                gfx.draw_pixel(flip_func(flip_x, x) + base_x,
                               flip_func(flip_y, y) + base_y,
                               palette_color(sprite, palette_number, val));
            }
        }
    }
}

void picture_processing_unit::draw_nametable(int index, int base_x) {
    constexpr auto num_rows = 30;
    constexpr auto num_tiles = 32;
    auto const base_address = index * 0x400;
    // Draw nametable zero
    for (auto row = 0; row < num_rows; ++row) {
        for (auto tile = 0; tile < num_tiles; ++tile) {
            auto const attribute = ram.at(base_address + 0x03C0 + tile / 4 +
                                          num_tiles / 4 * (row / 4));
            // Two bits per 2 x 2 tiles. Top left least significant, then top
            // right, bottom left, and finally bottom right.
            auto const shift_amount =
                ((row / 2) % 2) * 4 + ((tile / 2) % 2) * 2;
            auto const palette_number = (attribute >> shift_amount) & 0x03;
            draw_tile(tile * 8 + base_x, row * 8,
                      0x100 + ram.at(base_address + row * num_tiles + tile),
                      palette_number);
        }
    }
}

void picture_processing_unit::draw_sprites(int base_x) {
    constexpr size_t num_sprites = 64;
    constexpr size_t sprite_pitch = 4;
    constexpr size_t y_offset = 0;
    constexpr size_t tile_index_offset = 1;
    constexpr size_t attributes_offset = 2;
    constexpr size_t x_offset = 3;
    for (size_t i = 0; i < num_sprites; ++i) {
        // Sprites are offset by one in y.
        auto const y = oam.at(i * sprite_pitch + y_offset) + 1;
        auto const tile_index = oam.at(i * sprite_pitch + tile_index_offset);
        auto const attributes = oam.at(i * sprite_pitch + attributes_offset);
        auto const x = oam.at(i * sprite_pitch + x_offset);
        auto const palette_number = attributes & 0x03;
        draw_tile(base_x + x, y, tile_index, palette_number,
                  attributes & (1 << 6), attributes & (1 << 7), true);
    }
}

void picture_processing_unit::draw_debug() {
    constexpr auto num_tiles = 32;
    gfx.clear();
    // Draw left
    draw_tiles(0, 0, 0);
    // Draw right
    constexpr auto right_base_tile_index = 256;
    constexpr auto right_base_y = 17 * 9;
    draw_tiles(right_base_tile_index, 0, right_base_y);
    // Offset to clear debug CHR rom dump
    auto base_x = 17 * 9;
    draw_nametable(1, base_x);
    // Right of nametable one
    base_x += 8 * num_tiles + 9;
    draw_nametable(0, base_x);
    // Right of nametable one
    draw_sprites(base_x);
    gfx.flip();
}

void picture_processing_unit::draw() {
    gfx.clear();
    draw_nametable(0, 0);
    draw_sprites(0);
    gfx.flip();
}

uint8_t picture_processing_unit::read_PPUSTATUS() {
    logf(log_level::debug, "\tread PPUSTATUS");
    auto PPUSTATUS = vblank_started << 7;
    PPUSCROLL_latch = false;
    PPUADDR_latch = false;
    vblank_started = false;
    return PPUSTATUS;
}

void picture_processing_unit::set_PPUCTRL(uint8_t val) {
    logf(log_level::debug, "\twrite PPUCTRL %#04x", val);
    PPUCTRL = val;
}

void picture_processing_unit::set_PPUMASK(uint8_t val) {
    logf(log_level::debug, "\twrite PPUMASK %#04x", val);
    PPUMASK = val;
}

void picture_processing_unit::set_OAMADDR(uint8_t val) {
    logf(log_level::debug, "\twrite OAMADDR %#04x", val);
    OAMADDR = val;
}

void picture_processing_unit::write_PPUSCROLL(uint8_t val) {
    logf(log_level::debug, "\twrite PPUSCROLL");
    // TODO: Determine if it goes back and forth between X and Y
    // or if it goes X, Y, Y, Y ...
    if (!PPUSCROLL_latch) {
        PPUSCROLL_X = val;
        PPUSCROLL_latch = true;
    } else {
        PPUSCROLL_Y = val;
        PPUSCROLL_latch = false;
    }
}

void picture_processing_unit::write_PPUADDR(uint8_t val) {
    logf(log_level::debug, "\twrite PPUADDR %#04x", val);
    // TODO: Determine if it goes back and forth between X and Y
    // or if it goes X, Y, Y, Y ...
    if (!PPUADDR_latch) {
        assert(!PPUADDR_latch);
        logf(log_level::debug, " high");
        PPUADDR = (val << 8) + (0x00FF & PPUADDR);
        PPUADDR_latch = true;
    } else {
        logf(log_level::debug, " low");
        PPUADDR = (PPUADDR & 0xFF00) + val;
        PPUADDR_latch = false;
    }
}

void picture_processing_unit::write_PPUDATA(uint8_t val) {
    if (PPUADDR < 0x2000) {
        logf(log_level::debug, "\tTrying to write to PPU ROM");
    } else if (PPUADDR < 0x3000) {
        uint16_t mod_adr = PPUADDR - 0x2000;
        // Outside of VRAM?
        if (mod_adr > 0x07FF) {
            // Mirror
            mod_adr -= 0x0800;
        }
        logf(log_level::debug, "\t PPUDATA(%#6x)=%#4x", PPUADDR, val);
        ram[mod_adr] = val;
    } else if (PPUADDR >= 0x3f00 && PPUADDR < 0x3f20) {
        logf(log_level::debug, "\t PPUDATA(%#6x)=%#4x (palette)", PPUADDR, val);
        palette_data[PPUADDR - 0x3f00] = val;
        log(log_level::debug, "\npalette updated:\n\t");
        for (auto const d : palette_data) {
            log(log_level::debug, "{:02x}", d);
        }
        log(log_level::debug, "\n");
    } else {
        logf(log_level::error, "\nUnsupported PPUDATA write to %#6x\n",
             PPUADDR);
    }
    if (PPUCTRL & 0x04) {
        PPUADDR += 32;
    } else {
        PPUADDR += 1;
    }
}

void picture_processing_unit::dma_copy(std::span<uint8_t const, 0x100> data) {
    std::copy(
        std::begin(data),
        std::end(data),
        std::begin(oam));
}

bool picture_processing_unit::vblank() {
    log(log_level::debug, "vblank\n");
    vblank_started = true;
    if (PPUCTRL & 0x80) {
        // Should trigger nmi
        log(log_level::debug, "nmi triggered\n");
        return true;
    }
    log(log_level::debug, "nmi suppressed\n");
    return false;
}
