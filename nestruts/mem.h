#pragma once
#include <array>
#include <cstdint>
#include <memory>

#include "apu.h"
#include "controller.h"
#include "log.h"
#include "ppu.h"

class value_proxy;

class memory_bus final {
  public:
    memory_bus(std::shared_ptr<picture_processing_unit> ppu,
               std::shared_ptr<audio_processing_unit> apu,
               std::shared_ptr<controller> ctrl);
    void write(uint16_t adr, uint8_t val);
    uint8_t read(uint16_t adr);
    void load_rom(uint16_t adr, uint8_t val);

    value_proxy operator[](uint16_t adr);

  private:
    // 2 K of RAM
    std::array<uint8_t, 0x0800> ram{};
    // 32 K of ROM
    std::array<uint8_t, 0x08000> rom{};

    std::shared_ptr<picture_processing_unit> ppu;
    std::shared_ptr<audio_processing_unit> apu;
    std::shared_ptr<controller> ctrl;
};

// Behaves like a uint8_t for the user. When written to can either write to
// an underlying memory location or a locally stored value.
class value_proxy final {
  public:
    explicit value_proxy(uint8_t value) : m_value{value} {}
    explicit value_proxy(memory_bus *mem, uint16_t adr)
        : m_mem{mem}, m_adr{adr} {}
    void set(uint8_t val) {
        if (!m_mem) {
            // TODO: Make hard error or even compile time error.
            log(log_level::error, "Writing to read-only memory");
            return;
        }
        m_mem->write(m_adr, val);
    }
    uint8_t value() { return m_mem ? m_mem->read(m_adr) : m_value; }

  private:
    memory_bus *m_mem{};
    uint16_t m_adr{};
    uint8_t m_value{};
};
