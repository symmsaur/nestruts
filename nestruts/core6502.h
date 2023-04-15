#pragma once
#include "fmt/ostream.h"
#include "instruction_store.h"
#include "mem.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <ostream>

struct state {
    uint8_t status{};
    uint8_t accumulator{};
    uint8_t x{};
    uint8_t y{};
    uint8_t sp{};
    uint16_t pp{};
    bool faulted{};
    friend auto operator<=>(state const &lhs, state const &rhs) = default;
};

std::ostream &operator<<(std::ostream &stream, state);
template <> struct fmt::formatter<state> : fmt::ostream_formatter {};

class core6502 final {
  public:
    core6502(std::unique_ptr<memory_bus> bus, std::function<bool()> irq_func);
    void cycle();
    void interrupt();
    void nmi();
    void irq();

    void setpp(uint16_t new_pp);

    uint8_t get_acc();
    uint8_t get_x() { return x; };
    bool is_faulted();
    state dump_state();

  private:
    const uint16_t stack_offs{0x100};

    std::unique_ptr<memory_bus> const bus{};
    std::function<bool()> const irq_func{};

    // status register
    uint8_t status{};

    uint8_t accumulator{};
    // X register
    uint8_t x{};
    // Y register
    uint8_t y{};
    // stack register
    uint8_t sp{};

    uint16_t pp{};
    bool faulted{};

    int cycles_alloc{};

    instruction_info current_instruction{};
    instruction_store store{};

    uint8_t fetch();
    void push(uint8_t val);
    uint8_t pop();
    value_proxy bus_val(uint8_t opcode);
    void pushpp();

    void execute();

    // Memory access
    uint16_t zero(uint8_t adr);
    uint16_t zero_x(uint8_t adr);
    uint16_t zero_y(uint8_t adr);
    uint16_t absolute(uint8_t low, uint8_t high);
    uint16_t absolute_x(uint8_t low, uint8_t high);
    uint16_t absolute_y(uint8_t low, uint8_t high);
    uint16_t indirect_x(uint8_t adr);
    uint16_t indirect_y(uint8_t adr);
    uint16_t indirect(uint8_t low_adr, uint8_t high_adr);

    // Helpers
    void set_zero_flag(uint8_t val);
    void set_negative_flag(uint8_t val);
    void set_overflow_flag_add(uint8_t prev, uint8_t res);
    void compare(uint8_t reg, uint8_t val);
    void set_accumulator(uint8_t val);
    void set_x(uint8_t val);
    void set_y(uint8_t val);
    void set_sp(uint8_t val);
    void set_pp(uint16_t new_pp);

    // Instructions
    void LDA(uint8_t val);
    void LDY(uint8_t val);
    void LDX(uint8_t val);

    void STA(value_proxy val);
    void STX(value_proxy val);
    void STY(value_proxy val);

    void ADC(uint8_t val);
    void SBC(uint8_t val);
    void AND(uint8_t val);
    void ASL();
    void ASL(value_proxy val);
    uint8_t ASL(uint8_t val);
    void CMP(uint8_t val);
    void CPX(uint8_t val);
    void CPY(uint8_t val);
    void BIT(uint8_t val);
    void LSR();
    void LSR(value_proxy val);
    uint8_t LSR(uint8_t val);
    void ROL();
    void ROL(value_proxy val);
    uint8_t ROL(uint8_t val);
    void ROR();
    void ROR(value_proxy val);
    uint8_t ROR(uint8_t val);
    void ORA(uint8_t val);
    void EOR(uint8_t val);
    void INC(value_proxy val);
    void INX();
    void INY();
    void PHA();
    void PLA();
    void PHP();
    void PLP();
    void TAX();
    void TXA();
    void TAY();
    void TYA();
    void TXS();
    void DEC(value_proxy val);
    void DEX();
    void DEY();
    void SEC();
    void SEI();
    void CLC();
    void CLD();

    void BEQ(uint8_t val);
    void BMI(uint8_t val);
    void BNE(uint8_t val);
    void BCS(uint8_t val);
    void BCC(uint8_t val);
    void BPL(uint8_t val);
    void BVC(uint8_t val);
    void JSR(uint16_t adr);
    void JMP(uint16_t adr);
    void RTI();
    void RTS();
    void BRK();
};
