#pragma once
#include <cstdint>
#include <array>

#include "gfx.h"

class picture_processing_unit
{
public:
	picture_processing_unit();
	void load_rom(uint16_t adr, uint8_t val);
	void cycle();

	uint8_t read_PPUSTATUS();

	void set_PPUCTRL(uint8_t val);
	void set_PPUMASK(uint8_t val);
	void set_OAMADDR(uint8_t val);
	void write_PPUSCROLL(uint8_t val);
	void write_PPUADDR(uint8_t val);
	void write_PPUDATA(uint8_t val);

	void dma_write(uint8_t i, uint8_t val);

	// Should the NMI trigger
	bool NMI();
private:
	void draw_tiles(uint16_t base_offset, int base_x, int base_y);
	void draw_tile(int base_x, int base_y, uint16_t tile_index);
	void draw_nametable();
	void draw_sprites();
	void draw_debug();
	// 8 K of CHR ROM
	std::array<uint8_t, 0x2000> rom{};
	// 2 K of RAM
	std::array<uint8_t, 0x0800> ram{};
	// 256 B of OAM
	std::array<uint8_t, 0x0100> oam{};

	graphics gfx{};

	// registers
	uint8_t PPUCTRL = 0;
	uint8_t PPUMASK = 0;
	uint8_t OAMADDR = 0;
	uint8_t PPUSCROLL_X = 0;
	uint8_t PPUSCROLL_Y = 0;
	bool PPUSCROLL_latch = false;

	bool NMI_occured = false;

	uint16_t PPUADDR = 0;
	bool PPUADDR_latch = false;
	uint8_t PPUDATA_buffer = 0;

	int warmup = 0;
	int vblank = 0;
	int draw = 0;
};

