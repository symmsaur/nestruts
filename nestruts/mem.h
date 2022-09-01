#pragma once
#include <cstdint>
#include <array>
#include <memory>

#include "audio_processing_unit.h"
#include "ppu.h"


class memory_bus final
{
public:
	memory_bus(std::shared_ptr<picture_processing_unit> ppu, std::shared_ptr<audio_processing_unit> apu);
	void write(uint16_t adr, uint8_t val);
	uint8_t read(uint16_t adr);
	void load_rom(uint16_t adr, uint8_t val);
private:
	// 2 K of RAM
	std::array<uint8_t, 0x0800> ram;
	// 32 K of ROM
	std::array<uint8_t, 0x08000> rom;

	std::shared_ptr<picture_processing_unit> ppu;
	std::shared_ptr<audio_processing_unit> apu;
};

