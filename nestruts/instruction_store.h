#pragma once
#include <cstdint>
#include <map>
#include <string>

enum class adr_mode {
    implied,
    accumulator,
    immediate,
    absolute,
    x_indexed_absolute,
    y_indexed_absolute,
    absolute_indirect,
    zero_page,
    x_indexed_zero_page,
    y_indexed_zero_page,
    x_indexed_zero_page_indirect,
    zero_page_indirect_y_indexed,
    relative
};

class instruction_info {
    uint16_t m_pp{};
    std::string m_mnemonic{};
    adr_mode m_mode{};
    uint16_t m_argument{};

  public:
    void set_pp(uint16_t pp) { m_pp = pp; }
    void set_mnemonic(std::string mnemonic) {
        m_mnemonic = std::move(mnemonic);
    }
    void set_mode(adr_mode mode) { m_mode = mode; }
    void set_argument(std::uint16_t argument) { m_argument = argument; }

    uint16_t pp() const { return m_pp; }
    std::string mnemonic() const { return m_mnemonic; }
    adr_mode mode() const { return m_mode; }
    uint16_t argument() const { return m_argument; }
};

class instruction_store {
    std::map<uint16_t, instruction_info> m_instructions{};

  public:
    instruction_store() = default;
    instruction_store(instruction_store const &) = delete;
    instruction_store &operator=(instruction_store const &) = delete;
    instruction_store(instruction_store &&) = delete;
    instruction_store &operator=(instruction_store &&) = delete;
    ~instruction_store();

    void push(instruction_info instruction) {
        if (!m_instructions.contains(instruction.pp())) {
            m_instructions[instruction.pp()] = instruction;
        }
    }
};
