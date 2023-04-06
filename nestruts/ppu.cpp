#include "ppu.h"

#include <cassert>

#include "log.h"

picture_processing_unit::picture_processing_unit() : warmup{ 10000 } { }

void picture_processing_unit::load_rom(uint16_t adr, uint8_t val)
{
	rom.at(adr) = val;
}

void picture_processing_unit::cycle()
{
	if (warmup)
	{
		--warmup;
		if (!warmup)
		{
			draw_debug();
			log(log_level::debug, "warmup done\n");
			draw = 1000000;
		}
		if ((warmup % 1000) == 0)
		{
			log(log_level::debug, "warmup %i cycles left\n", warmup);
		}
		return;
	}
	if (draw)
	{
		--draw;
		if (!draw)
		{
			vblank = 100000;
			log(log_level::debug, "draw -> vblank\n");
			NMI_occured = true;
		}
		return;
	}
	if (vblank)
	{
		--vblank;
		if (!vblank)
		{
			draw_debug();
			NMI_occured = false;
			log(log_level::debug, "vblank -> draw\n");
			draw = 1000000;
		}
		return;
	}
}

void picture_processing_unit::draw_tiles(uint16_t base_tile_index, int base_x, int base_y)
{
	for (auto i = 0; i < 16; i++)
	{
		for (auto j = 0; j < 16; j++)
		{
			auto x = i * 9 + base_x;
			auto y = j * 9 + base_y;
			draw_tile(x, y, (j * 16 + i) + base_tile_index);
		}
	}
}

void picture_processing_unit::draw_tile(int base_x, int base_y, uint16_t tile_index)
{
	auto offset = tile_index * 16;
	for (auto y = 0; y < 8; y++)
	{
		// Two bit planes for the tile
		uint8_t low_bits = rom.at(offset + y);
		uint8_t high_bits = rom.at(offset + y + 8);
		for (auto x = 0; x < 8; x++)
		{
			// Take out one high and one low bit from the left.
			auto val = (low_bits >> 7) + (high_bits >> 7) * 2;
			low_bits = low_bits << 1;
			high_bits = high_bits << 1;
			gfx.draw_pixel(x + base_x, y + base_y, val);
		}
	}
}

void picture_processing_unit::draw_nametable()
{
	constexpr auto num_rows = 30;
	constexpr auto num_tiles = 32;
	// Offset to clear debug CHR rom dump
	auto base_x = 17 * 9;
	// Draw nametable one.
	for (auto row = 0; row < num_rows; ++row)
	{
		for (auto tile = 0; tile < num_tiles; ++tile)
		{
			draw_tile(tile * 8 + base_x, row * 8, 0x100 + ram.at(0x400 + row * num_tiles + tile));
		}
	}
	// Right of nametable one
	base_x += 8 * num_tiles + 9; 
	// Draw nametable zero
	for (auto row = 0; row < num_rows; ++row)
	{
		for (auto tile = 0; tile < num_tiles; ++tile)
		{
			draw_tile(tile * 8 + base_x, row * 8, 0x100 + ram.at(0x0 + row * num_tiles + tile));
		}
	}
}

void picture_processing_unit::draw_sprites()
{
	constexpr size_t num_sprites = 64;
	constexpr size_t sprite_pitch = 4;
	constexpr size_t y_offset = 0;
	constexpr size_t tile_index_offset = 1;
	// TODO: Attributes are unused
	// constexpr size_t attributes_offset = 2;
	constexpr size_t x_offset = 3;
	// Refactor this magical thing to get it to draw in the correct place
	constexpr auto base_x = 8 * 32 + 9 + 17 * 9;
	for (size_t i = 0; i < num_sprites; ++i)
	{
		// Sprites are offset by one in y.
		auto const y = oam.at(i * sprite_pitch + y_offset) + 1;
		auto const tile_index = oam.at(i * sprite_pitch + tile_index_offset);
		// TODO: Attributes are unused
		//auto const attributes = oam.at(i * sprite_pitch + attributes_offset);
		auto const x = oam.at(i * sprite_pitch + x_offset);
		draw_tile(base_x + x, y, tile_index);

	}
}

void picture_processing_unit::draw_debug()
{
	// Draw left
	draw_tiles(0, 0, 0);
	// Draw right
	constexpr auto right_base_tile_index = 256;
	constexpr auto right_base_y = 17 * 9;
	draw_tiles(right_base_tile_index, 0, right_base_y);
	draw_nametable();
	draw_sprites();
	gfx.flip();
}

uint8_t picture_processing_unit::read_PPUSTATUS()
{
	log(log_level::debug, "\tread PPUSTATUS");
	auto PPUSTATUS = NMI_occured << 7;
	PPUSCROLL_latch = false;
	PPUADDR_latch = false;
	NMI_occured = false;
	return PPUSTATUS;
}

void picture_processing_unit::set_PPUCTRL(uint8_t val)
{
	log(log_level::debug, "\twrite PPUCTRL %#04x", val);
	PPUCTRL = val;
}

void picture_processing_unit::set_PPUMASK(uint8_t val)
{
	log(log_level::debug, "\twrite PPUMASK %#04x", val);
	PPUMASK = val;
}

void picture_processing_unit::set_OAMADDR(uint8_t val)
{
	log(log_level::debug, "\twrite OAMADDR %#04x", val);
	OAMADDR = val;
}

void picture_processing_unit::write_PPUSCROLL(uint8_t val)
{
	log(log_level::debug, "\twrite PPUSCROLL");
	// TODO: Determine if it goes back and forth between X and Y
	// or if it goes X, Y, Y, Y ...
	if (!PPUSCROLL_latch) {
		PPUSCROLL_X = val;
		PPUSCROLL_latch = true;
	}
	else {
		PPUSCROLL_Y = val;
		PPUSCROLL_latch = false;
	}

}

void picture_processing_unit::write_PPUADDR(uint8_t val)
{
	log(log_level::debug, "\twrite PPUADDR %#04x", val);
	// TODO: Determine if it goes back and forth between X and Y
	// or if it goes X, Y, Y, Y ...
	if (!PPUADDR_latch) {
		assert(!PPUADDR_latch);
		log(log_level::debug, " high");
		PPUADDR = (val << 8) + (0x00FF & PPUADDR);
		PPUADDR_latch = true;
	}
	else {
		log(log_level::debug, " low");
		PPUADDR = (PPUADDR & 0xFF00) + val;
		PPUADDR_latch = false;
	}
}

void picture_processing_unit::write_PPUDATA(uint8_t val)
{
	if (PPUADDR < 0x2000) {
		log(log_level::debug, "\tTrying to write to PPU ROM");
	}
	else if (PPUADDR < 0x3000) {
		uint16_t mod_adr = PPUADDR - 0x2000;
		// Outside of VRAM?
		if (mod_adr > 0x07FF)
		{
			// Mirror
			mod_adr -= 0x0800;
		}
		log(log_level::debug, "\t PPUDATA(%#6x)=%#4x", PPUADDR, val);
		ram[mod_adr] = val;
		//draw_debug();
	}
	else
	{
		log(log_level::error, "\nUnsupported PPUDATA write to %#6x\n", PPUADDR);
	}
	if (PPUCTRL & 0x04) {
		PPUADDR += 32;
	}
	else {
		PPUADDR += 1;
	}
}

void picture_processing_unit::dma_write(uint8_t i, uint8_t val)
{
	// Is this correct with respect to OAMADDR?
	oam[i] = val;
}

bool picture_processing_unit::NMI()
{
	if (NMI_occured && PPUCTRL & 0x80)
	{
		return true;
	}
	else
	{
		return false;
	}
}
