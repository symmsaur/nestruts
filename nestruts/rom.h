#pragma once

#include <string>

#include "mem.h"
#include "ppu.h"


void load_rom(std::string const& filename, picture_processing_unit& ppu,
         memory_bus& mem);
