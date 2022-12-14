#pragma once
#include "mem.h"
#include <memory>
#include <cstdint>
#include <functional>

class core6502 final
{
public:
	core6502(std::unique_ptr<memory_bus> bus, std::function<bool()> nmi_func, std::function<bool()> irq_func);
	void cycle();
	void interrupt();
	void nmi();
	void irq();

	void setpp(uint16_t new_pp);
	uint8_t get_acc();
	bool is_faulted();

private:
	const uint16_t stack_offs{ 0x100 };

	std::unique_ptr<memory_bus> const bus{};
	std::function<bool()> const nmi_func{};
	std::function<bool()> const irq_func{};
	bool nmi_low{};

	// status register
	uint8_t status{};

	uint8_t accumulator{};
	// X register
	uint8_t x{};
	// Y resgister
	uint8_t y{};
	// stack register
	uint8_t sp{};

	uint16_t pp{};
	bool faulted{};

	int cycles_alloc{};

	uint8_t fetch();
	void push(uint8_t val);
	uint8_t pop();
	uint8_t bus_val(uint8_t opcode);
	uint16_t tgt_adr(uint8_t opcode);
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

	void STA(uint16_t adr);
	void STX(uint16_t adr);
	void STY(uint16_t adr);

	void ADC(uint8_t val);
	void SBC(uint8_t val);
	void AND(uint8_t val);
	void ASL();
	void ASL(uint16_t adr);
	uint8_t ASL(uint8_t val);
	void CMP(uint8_t val);
	void CPX(uint8_t val);
	void CPY(uint8_t val);
	void BIT(uint8_t val);
	void LSR();
	void LSR(uint16_t adr);
	uint8_t LSR(uint8_t val);
	void ROL();
	void ROL(uint16_t adr);
	uint8_t ROL(uint8_t val);
	void ROR();
	void ROR(uint16_t adr);
	uint8_t ROR(uint8_t val);
	void ORA(uint8_t val);
	void EOR(uint8_t val);
	void INC(uint16_t adr);
	void INX();
	void INY();
	void PHA();
	void PLA();
	void PHP();
	void TAX();
	void TXA();
	void TAY();
	void TYA();
	void TXS();
	void DEC(uint16_t adr);
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

