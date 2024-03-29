#include "mem.h"

#include "log.h"
#include <cstdint>
#include <memory>

memory_bus::memory_bus(std::shared_ptr<picture_processing_unit> p,
                       std::shared_ptr<audio_processing_unit> a,
                       std::shared_ptr<controller> c)
    : ram{}, rom{}, ppu{std::move(p)}, apu{std::move(a)}, ctrl{std::move(c)} {}

void memory_bus::write(uint16_t adr, uint8_t val) {
    // RAM
    if (adr < 0x2000) {
        logf(log_level::debug, "\tw %#06x=%#04x ", adr, val);
        uint16_t mod_adr = adr % 0x800;
        ram[mod_adr] = val;
    }
    // PPU
    else if (adr < 0x4000) {
        uint16_t ppu_reg = adr % 0x8;
        switch (ppu_reg) {
        case 0x0:
            ppu->set_PPUCTRL(val);
            break;
        case 0x1:
            ppu->set_PPUMASK(val);
            break;
        case 0x2:
            logf(log_level::error, "unimplemented ppu write: %d\n", ppu_reg);
            break;
        case 0x3:
            ppu->set_OAMADDR(val);
            break;
        case 0x4:
            logf(log_level::error, "unimplemented ppu write: %d\n", ppu_reg);
            break;
        case 0x5:
            ppu->write_PPUSCROLL(val);
            break;
        case 0x6:
            ppu->write_PPUADDR(val);
            break;
        case 0x7:
            ppu->write_PPUDATA(val);
            break;
        }
        // MISC
    } else if (adr == 0x4000) {
        apu->pulse1.dlcn(val);
    } else if (adr == 0x4001) {
        apu->pulse1.sweep(val);
    } else if (adr == 0x4002) {
        apu->pulse1.timer_low(val);
    } else if (adr == 0x4003) {
        apu->pulse1.length_counter_timer_high(val);
    } else if (adr == 0x4004) {
        apu->pulse2.dlcn(val);
    } else if (adr == 0x4005) {
        apu->pulse2.sweep(val);
    } else if (adr == 0x4006) {
        apu->pulse2.timer_low(val);
    } else if (adr == 0x4007) {
        apu->pulse2.length_counter_timer_high(val);
    } else if (adr == 0x4014) {
        // OAM DMA
        // Should take 513 or 514 cycles.
        std::size_t offs = val << 8;
        log(log_level::debug, "\toam copy from [${:04x}-${:04x}]", offs, offs + 0xFF);
        ppu->dma_copy(std::span<uint8_t, 0x100>(ram.data() + offs, 0x100));
    } else if (adr == 0x415) {
        apu->write_status(adr);
    } else if (adr == 0x4016) {
        ctrl->write(val);
    } else if (adr == 0x4017) {
        logf(log_level::debug, "\tWrite APU frame counter");
        apu->set_frame_counter(val);
    } else if ((adr >= 0x4000 && adr <= 0x4013) || adr == 0x4015) {
        logf(log_level::debug, "\tWriting to unimplemented APU: %#6x\n", adr);
    } else if (adr >= 0x8000) {
        logf(log_level::error, "\tWarning: trying to write to ROM: %#6x\n",
             adr);
    } else {
        logf(log_level::error, "\tUnsupported bus write to: %#6x\n", adr);
    }
}

uint8_t memory_bus::read(uint16_t adr) {
    // RAM
    if (adr < 0x2000) {
        uint16_t mod_adr = adr % 0x800;
        logf(log_level::debug, "r %#06x=%#04x ", adr, ram[mod_adr]);
        return ram[mod_adr];
    }
    // PPU
    else if (adr < 0x4000) {
        uint16_t ppu_reg = adr % 0x8;
        switch (ppu_reg) {
        case 0x2:
            return ppu->read_PPUSTATUS();
        default:
            logf(log_level::error, "Unsupported ppu read\n");
            return 0;
        }
        // MISC
    } else if (adr == 0x4015) {
        return apu->read_status();
    } else if (adr == 0x4016) {
        return ctrl->read();
    } else if (adr == 0x4017) {
        logf(log_level::debug,
             "\t Reading from unimplemented controller: %#6x\n", adr);
        // Assume 0 is ok to return.
        return 0;
    } else if (adr >= 0x8000) {
        // Remove base address
        uint16_t mod_adr = adr - 0x8000;
        return rom[mod_adr];
    } else {
        logf(log_level::error, "\tUnsupported read : %#6x\n", adr);
        return 0;
    }
}

value_proxy memory_bus::operator[](uint16_t adr) {
    return value_proxy{this, adr};
}

void memory_bus::load_rom(uint16_t adr, uint8_t val) {
    if (adr == 0xFFFC) {
        logf(log_level::debug, "Writing reset vector low %#04x\n", val);
    } else if (adr == 0xFFFD) {
        logf(log_level::debug, "Writing reset vector high %#04x\n", val);
    }
    // Remove base address
    uint16_t mod_adr = adr - 0x8000;
    rom[mod_adr] = val;
}
