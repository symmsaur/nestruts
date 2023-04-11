#include "core6502.h"

#include "log.h"

namespace {
constexpr uint8_t carry_flag = 1;
constexpr uint8_t zero_flag = 1 << 1;
constexpr uint8_t interrupt_disable_flag = 1 << 2;
constexpr uint8_t decimal_flag = 1 << 3;
// Bits 4 and 5 are unused.
constexpr uint8_t overflow_flag = 1 << 6;
constexpr uint8_t negative_flag = 1 << 7;
} // namespace

std::ostream &operator<<(std::ostream &stream, state s) {
    stream << "State {\n";

    stream << "  status: ";
    stream << (s.status & negative_flag ? "N" : "-");
    stream << (s.status & overflow_flag ? "O" : "-");
    stream << "--"; // Bits 4 and 5
    stream << (s.status & decimal_flag ? "D" : "-");
    stream << (s.status & interrupt_disable_flag ? "I" : "-");
    stream << (s.status & zero_flag ? "Z" : "-");
    stream << (s.status & carry_flag ? "C" : "-");
    stream << "\n";

    stream << "  accumulator: " << static_cast<int>(s.accumulator) << "\n";
    stream << "  x: " << static_cast<int>(s.x) << "\n";
    stream << "  y: " << static_cast<int>(s.y) << "\n";
    stream << "  sp: " << static_cast<int>(s.sp) << "\n";
    stream << "  pp: $" << std::hex << s.pp << "\n";
    stream << "  faulted: " << s.faulted << "\n";
    stream << "}";
    return stream;
}

core6502::core6502(std::unique_ptr<memory_bus> bus,
                   std::function<bool()> irq_func)
    : bus{std::move(bus)}, irq_func{irq_func}, sp{0xff} {
    log(log_level::debug, "Created core6502\n");
}

void core6502::cycle() {
    interrupt();
    execute();
}

void core6502::interrupt() {
    if (irq_func() && !(status & interrupt_disable_flag)) {
        irq();
    }
}

void core6502::nmi() {
    logf(log_level::instr, "Triggered NMI\n");
    logf(log_level::debug, "\t push pp");
    pushpp();
    logf(log_level::debug, "\t push status ");
    push(status);
    logf(log_level::debug, "\n");
    uint16_t addr = bus->read(0xFFFA) + (bus->read(0xFFFB) << 8);
    set_pp(addr);
}

void core6502::irq() {
    logf(log_level::instr, "Triggered IRQ\n");
    logf(log_level::debug, "\t push pp");
    pushpp();
    logf(log_level::debug, "\t push status ");
    push(status);
    logf(log_level::debug, "\n");
    uint16_t addr = bus->read(0xFFFE) + (bus->read(0xFFFF) << 8);
    logf(log_level::debug, "jumping to %06x\n", addr);
    set_pp(addr);
    logf(log_level::debug, "setting interrupt disable status flag\n");
    status |= interrupt_disable_flag;
}

void core6502::setpp(uint16_t new_pp) { pp = new_pp; }

uint8_t core6502::get_acc() { return accumulator; }

state core6502::dump_state() {
    state s{};
    s.status = status;
    s.accumulator = accumulator;
    s.x = x;
    s.y = y;
    s.sp = sp;
    s.pp = pp;
    s.faulted = faulted;
    return s;
}
bool core6502::is_faulted() { return faulted; }

uint8_t core6502::fetch() {
    uint8_t val = bus->read(pp++);
    return val;
}

void core6502::push(uint8_t val) {
    bus->write(stack_offs + sp, val);
    set_sp(sp - 1);
}

uint8_t core6502::pop() {
    set_sp(sp + 1);
    return bus->read(stack_offs + sp);
}

uint8_t core6502::bus_val(uint8_t opcode) {
    logf(log_level::instr, " ");
    switch (opcode) {
    case 0xA9:
    case 0x69:
    case 0x29:
    case 0xA0:
    case 0xA2:
    case 0x09:
    case 0xC9:
    case 0xE0:
    case 0xC0:
    case 0x49:
    case 0xE9: {
        uint8_t arg = fetch();
        logf(log_level::instr, "#%02x", arg);
        current_instruction.set_mode(adr_mode::immediate);
        current_instruction.set_argument(arg);
        return arg;
    }
    case 0x30:
    case 0x10:
    case 0x50:
    case 0x90:
    case 0xB0:
    case 0xD0:
    case 0xF0: {
        uint8_t arg = fetch();
        logf(log_level::instr, "*%i", static_cast<int8_t>(arg));
        current_instruction.set_mode(adr_mode::relative);
        current_instruction.set_argument(arg);
        return arg;
    }
    case 0xA5:
    case 0x65:
    case 0x25:
    case 0xA4:
    case 0xA6:
    case 0x05:
    case 0xC5:
    case 0xE4:
    case 0xC4:
    case 0x24:
    case 0x45:
    case 0xE5: {
        uint8_t arg = fetch();
        logf(log_level::instr, "$%02x", arg);
        current_instruction.set_mode(adr_mode::zero_page);
        current_instruction.set_argument(arg);
        return bus->read(zero(arg));
    }
    case 0xB5:
    case 0x75:
    case 0x35:
    case 0xB4:
    case 0x15:
    case 0xD5:
    case 0x55:
    case 0xF5: {
        uint8_t arg = fetch();
        logf(log_level::instr, "$%02x,X", arg);
        current_instruction.set_mode(adr_mode::x_indexed_zero_page);
        current_instruction.set_argument(arg);
        return bus->read(zero_x(arg));
    }
    case 0xB6: {
        uint8_t arg = fetch();
        logf(log_level::instr, "$%02x,Y", arg);
        current_instruction.set_mode(adr_mode::y_indexed_zero_page);
        current_instruction.set_argument(arg);
        return bus->read(zero_y(arg));
    }
    case 0xAD:
    case 0x6D:
    case 0x2D:
    case 0xAC:
    case 0xAE:
    case 0x0D:
    case 0xCD:
    case 0xEC:
    case 0xCC:
    case 0x2C:
    case 0x4D:
    case 0xED: {
        uint8_t low_adr = fetch();
        uint8_t high_adr = fetch();
        logf(log_level::instr, "$%02x%02x", high_adr, low_adr);
        current_instruction.set_mode(adr_mode::absolute);
        current_instruction.set_argument((high_adr << 8) + low_adr);
        return bus->read(absolute(low_adr, high_adr));
    }
    case 0xBD:
    case 0x7D:
    case 0x3D:
    case 0xBC:
    case 0x1D:
    case 0xDD:
    case 0x5D:
    case 0xFD: {
        uint8_t low_adr = fetch();
        uint8_t high_adr = fetch();
        logf(log_level::instr, "$%02x%02x,X", high_adr, low_adr);
        current_instruction.set_mode(adr_mode::x_indexed_absolute);
        current_instruction.set_argument((high_adr << 8) + low_adr);
        return bus->read(absolute_x(low_adr, high_adr));
    }
    case 0xB9:
    case 0x79:
    case 0x39:
    case 0xBE:
    case 0x19:
    case 0xD9:
    case 0x59:
    case 0xF9: {
        uint8_t low_adr = fetch();
        uint8_t high_adr = fetch();
        logf(log_level::instr, "$%02x%02x,Y", high_adr, low_adr);
        current_instruction.set_mode(adr_mode::y_indexed_absolute);
        current_instruction.set_argument((high_adr << 8) + low_adr);
        return bus->read(absolute_y(low_adr, high_adr));
    }
    case 0xA1:
    case 0x61:
    case 0x21:
    case 0x01:
    case 0xC1:
    case 0x41:
    case 0xE1: {
        uint8_t arg = fetch();
        logf(log_level::instr, "($%02x,X)", arg);
        current_instruction.set_mode(adr_mode::x_indexed_zero_page_indirect);
        current_instruction.set_argument(arg);
        return bus->read(indirect_x(arg));
    }
    case 0xB1:
    case 0x71:
    case 0x31:
    case 0x11:
    case 0xD1:
    case 0x51:
    case 0xF1: {
        uint8_t arg = fetch();
        logf(log_level::instr, "($%02x),Y", arg);
        current_instruction.set_mode(adr_mode::zero_page_indirect_y_indexed);
        current_instruction.set_argument(arg);
        return bus->read(indirect_y(arg));
    }
    default:
        logf(log_level::error, "Unsupported opcode for bus_val %#4x", opcode);
        faulted = true;
        return 0u;
    }
    logf(log_level::instr, " ");
}
uint16_t core6502::tgt_adr(uint8_t opcode) {
    logf(log_level::instr, " ");
    switch (opcode) {
    case 0x06:
    case 0x85:
    case 0x86:
    case 0x46:
    case 0xE6:
    case 0x84:
    case 0xC6:
    case 0x66:
    case 0x26: {
        uint8_t arg = fetch();
        logf(log_level::instr, "$%02x", arg);
        current_instruction.set_mode(adr_mode::zero_page);
        current_instruction.set_argument(arg);
        return zero(arg);
    }
    case 0x16:
    case 0x95:
    case 0x96:
    case 0x56:
    case 0xF6:
    case 0x94:
    case 0xD6:
    case 0x76:
    case 0x36: {
        uint8_t arg = fetch();
        logf(log_level::instr, "$%02x,X", arg);
        current_instruction.set_mode(adr_mode::x_indexed_zero_page);
        current_instruction.set_argument(arg);
        return zero_x(arg);
    }
    case 0x0E:
    case 0x8D:
    case 0x8E:
    case 0x20:
    case 0x4E:
    case 0x4C:
    case 0xEE:
    case 0x8C:
    case 0xCE:
    case 0x6E:
    case 0x2E: {
        uint8_t low_adr = fetch();
        uint8_t high_adr = fetch();
        logf(log_level::instr, "$%02x%02x", high_adr, low_adr);
        current_instruction.set_mode(adr_mode::absolute);
        current_instruction.set_argument((high_adr << 8) + low_adr);
        return absolute(low_adr, high_adr);
    }
    case 0x1E:
    case 0x9D:
    case 0x5E:
    case 0xFE:
    case 0xDE:
    case 0x7E:
    case 0x3E: {
        uint8_t low_adr = fetch();
        uint8_t high_adr = fetch();
        logf(log_level::instr, "$%02x%02x,X", high_adr, low_adr);
        current_instruction.set_mode(adr_mode::x_indexed_absolute);
        current_instruction.set_argument((high_adr << 8) + low_adr);
        return absolute_x(low_adr, high_adr);
    }
    case 0x99: {
        uint8_t low_adr = fetch();
        uint8_t high_adr = fetch();
        logf(log_level::instr, " $%02x%02x,Y", high_adr, low_adr);
        current_instruction.set_mode(adr_mode::y_indexed_absolute);
        current_instruction.set_argument((high_adr << 8) + low_adr);
        return absolute_y(low_adr, high_adr);
    }
    case 0x81: {
        uint8_t arg = fetch();
        logf(log_level::instr, "($%02x,X)", arg);
        current_instruction.set_mode(adr_mode::x_indexed_zero_page_indirect);
        current_instruction.set_argument(arg);
        return indirect_x(arg);
    }
    case 0x91: {
        uint8_t arg = fetch();
        logf(log_level::instr, "($%02x),Y", arg);
        current_instruction.set_mode(adr_mode::zero_page_indirect_y_indexed);
        current_instruction.set_argument(arg);
        return indirect_y(arg);
    }
    case 0x6C: {
        uint8_t low_adr = fetch();
        uint8_t high_adr = fetch();
        logf(log_level::instr, "($%02x%02x)", high_adr, low_adr);
        current_instruction.set_mode(adr_mode::absolute_indirect);
        current_instruction.set_argument((high_adr << 8) + low_adr);
        return indirect(low_adr, high_adr);
    }
    default:
        logf(log_level::error, "Failure to get tgt_adr for opcode %#4x",
             opcode);
        faulted = true;
        return 0u;
    }
}

void core6502::execute() {
    current_instruction = {};
    current_instruction.set_pp(pp);
    logf(log_level::instr, "%#06x: ", pp);
    uint8_t opcode = fetch();
    switch (opcode) {
    case 0xA9:
    case 0xA5:
    case 0xB5:
    case 0xAD:
    case 0xBD:
    case 0xB9:
    case 0xA1:
    case 0xB1:
        logf(log_level::instr, "LDA");
        current_instruction.set_mnemonic("LDA");
        LDA(bus_val(opcode));
        break;
    case 0xA0:
    case 0xA4:
    case 0xB4:
    case 0xAC:
    case 0xBC:
        logf(log_level::instr, "LDY");
        current_instruction.set_mnemonic("LDY");
        LDY(bus_val(opcode));
        break;
    case 0xA2:
    case 0xA6:
    case 0xB6:
    case 0xAE:
    case 0xBE:
        logf(log_level::instr, "LDX");
        current_instruction.set_mnemonic("LDX");
        LDX(bus_val(opcode));
        break;
    case 0x69:
    case 0x65:
    case 0x75:
    case 0x6D:
    case 0x7D:
    case 0x79:
    case 0x61:
    case 0x71:
        logf(log_level::instr, "ADC");
        current_instruction.set_mnemonic("ADC");
        ADC(bus_val(opcode));
        break;
    case 0xE9:
    case 0xE5:
    case 0xF5:
    case 0xED:
    case 0xFD:
    case 0xF9:
    case 0xE1:
    case 0xF1:
        logf(log_level::instr, "SBC");
        current_instruction.set_mnemonic("SBC");
        SBC(bus_val(opcode));
        break;
    case 0x29:
    case 0x25:
    case 0x35:
    case 0x2D:
    case 0x3D:
    case 0x39:
    case 0x21:
    case 0x31:
        logf(log_level::instr, "AND");
        current_instruction.set_mnemonic("AND");
        AND(bus_val(opcode));
        break;
    case 0x09:
    case 0x05:
    case 0x15:
    case 0x0D:
    case 0x1D:
    case 0x19:
    case 0x01:
    case 0x11:
        logf(log_level::instr, "ORA");
        current_instruction.set_mnemonic("ORA");
        ORA(bus_val(opcode));
        break;
    case 0x49:
    case 0x45:
    case 0x55:
    case 0x4D:
    case 0x5D:
    case 0x59:
    case 0x41:
    case 0x51:
        logf(log_level::instr, "EOR");
        current_instruction.set_mnemonic("EOR");
        EOR(bus_val(opcode));
        break;
    case 0xC9:
    case 0xC5:
    case 0xD5:
    case 0xCD:
    case 0xDD:
    case 0xD9:
    case 0xC1:
    case 0xD1:
        logf(log_level::instr, "CMP");
        current_instruction.set_mnemonic("CMP");
        CMP(bus_val(opcode));
        break;
    case 0xE0:
    case 0xE4:
    case 0xEC:
        logf(log_level::instr, "CPX");
        current_instruction.set_mnemonic("CPX");
        CPX(bus_val(opcode));
        break;
    case 0xC0:
    case 0xC4:
    case 0xCC:
        logf(log_level::instr, "CPY");
        current_instruction.set_mnemonic("CPY");
        CPY(bus_val(opcode));
        break;
    case 0x24:
    case 0x2C:
        logf(log_level::instr, "BIT");
        current_instruction.set_mnemonic("BIT");
        BIT(bus_val(opcode));
        break;
    case 0x0A:
        logf(log_level::instr, "ASL");
        current_instruction.set_mnemonic("ASL");
        current_instruction.set_mode(adr_mode::accumulator);
        ASL();
        break;
    case 0x06:
    case 0x16:
    case 0x0E:
    case 0x1E:
    case 0x46:
    case 0x56:
    case 0x4E:
    case 0x5E:
        logf(log_level::instr, "ASL");
        current_instruction.set_mnemonic("ASL");
        ASL(tgt_adr(opcode));
        break;
    case 0x4A:
        logf(log_level::instr, "LSR");
        current_instruction.set_mnemonic("LSR");
        current_instruction.set_mode(adr_mode::accumulator);
        LSR();
        break;
    case 0x2A:
        logf(log_level::instr, "ROL");
        current_instruction.set_mnemonic("ROL");
        current_instruction.set_mode(adr_mode::accumulator);
        ROL();
        break;
    case 0x26:
    case 0x36:
    case 0x2E:
    case 0x3E:
        logf(log_level::instr, "ROL");
        current_instruction.set_mnemonic("ROL");
        ROL(tgt_adr(opcode));
        break;
    case 0x6A:
        logf(log_level::instr, "ROR");
        current_instruction.set_mnemonic("ROR");
        current_instruction.set_mode(adr_mode::accumulator);
        ROR();
        break;
    case 0x66:
    case 0x76:
    case 0x6E:
    case 0x7E:
        logf(log_level::instr, "ROR");
        current_instruction.set_mnemonic("ROR");
        ROR(tgt_adr(opcode));
        break;
    case 0x85:
    case 0x95:
    case 0x8D:
    case 0x9D:
    case 0x99:
    case 0x81:
    case 0x91:
        logf(log_level::instr, "STA");
        current_instruction.set_mnemonic("STA");
        STA(tgt_adr(opcode));
        break;
    case 0x86:
    case 0x96:
    case 0x8E:
        logf(log_level::instr, "STX");
        current_instruction.set_mnemonic("STX");
        STX(tgt_adr(opcode));
        break;
    case 0x84:
    case 0x94:
    case 0x8C:
        logf(log_level::instr, "STY");
        current_instruction.set_mnemonic("STY");
        STY(tgt_adr(opcode));
        break;
    case 0xF0:
        logf(log_level::instr, "BEQ");
        current_instruction.set_mnemonic("BEQ");
        BEQ(bus_val(opcode));
        break;
    case 0x30:
        logf(log_level::instr, "BMI");
        current_instruction.set_mnemonic("BMI");
        BMI(bus_val(opcode));
        break;
    case 0xD0:
        logf(log_level::instr, "BNE");
        current_instruction.set_mnemonic("BNE");
        BNE(bus_val(opcode));
        break;
    case 0xB0:
        logf(log_level::instr, "BCS");
        current_instruction.set_mnemonic("BCS");
        BCS(bus_val(opcode));
        break;
    case 0x90:
        logf(log_level::instr, "BCC");
        current_instruction.set_mnemonic("BCC");
        BCC(bus_val(opcode));
        break;
    case 0x10:
        logf(log_level::instr, "BPL");
        current_instruction.set_mnemonic("BPL");
        BPL(bus_val(opcode));
        break;
    case 0x50:
        logf(log_level::instr, "BVC");
        current_instruction.set_mnemonic("BVC");
        BVC(bus_val(opcode));
        break;
    case 0x20:
        logf(log_level::instr, "JSR");
        current_instruction.set_mnemonic("JSR");
        JSR(tgt_adr(opcode));
        break;
    case 0x4C:
    case 0x6C:
        logf(log_level::instr, "JMP");
        current_instruction.set_mnemonic("JMP");
        JMP(tgt_adr(opcode));
        break;
    case 0x40:
        logf(log_level::instr, "RTI");
        current_instruction.set_mnemonic("RTI");
        RTI();
        break;
    case 0x60:
        logf(log_level::instr, "RTS");
        current_instruction.set_mnemonic("RTS");
        RTS();
        break;
    case 0xE6:
    case 0xF6:
    case 0xEE:
    case 0xFE:
        logf(log_level::instr, "INC");
        current_instruction.set_mnemonic("INC");
        INC(tgt_adr(opcode));
        break;
    case 0xC6:
    case 0xD6:
    case 0xCE:
    case 0xDE:
        logf(log_level::instr, "DEC");
        current_instruction.set_mnemonic("DEC");
        DEC(tgt_adr(opcode));
        break;
    case 0xE8:
        logf(log_level::instr, "INX");
        current_instruction.set_mnemonic("INX");
        current_instruction.set_mode(adr_mode::implied);
        INX();
        break;
    case 0xC8:
        logf(log_level::instr, "INY");
        current_instruction.set_mnemonic("INY");
        current_instruction.set_mode(adr_mode::implied);
        INY();
        break;
    case 0x48:
        logf(log_level::instr, "PHA");
        current_instruction.set_mnemonic("PHA");
        current_instruction.set_mode(adr_mode::implied);
        PHA();
        break;
    case 0x68:
        logf(log_level::instr, "PLA");
        current_instruction.set_mnemonic("PLA");
        current_instruction.set_mode(adr_mode::implied);
        PLA();
        break;
    case 0xAA:
        logf(log_level::instr, "TAX");
        current_instruction.set_mnemonic("TAX");
        current_instruction.set_mode(adr_mode::implied);
        TAX();
        break;
    case 0x8A:
        logf(log_level::instr, "TXA");
        current_instruction.set_mnemonic("TXA");
        current_instruction.set_mode(adr_mode::implied);
        TXA();
        break;
    case 0xA8:
        logf(log_level::instr, "TAY");
        current_instruction.set_mnemonic("TAY");
        current_instruction.set_mode(adr_mode::implied);
        TAY();
        break;
    case 0x98:
        logf(log_level::instr, "TYA");
        current_instruction.set_mnemonic("TYA");
        current_instruction.set_mode(adr_mode::implied);
        TYA();
        break;
    case 0x9A:
        logf(log_level::instr, "TXS");
        current_instruction.set_mnemonic("TXS");
        current_instruction.set_mode(adr_mode::implied);
        TXS();
        break;
    case 0xCA:
        logf(log_level::instr, "DEX");
        current_instruction.set_mnemonic("DEX");
        current_instruction.set_mode(adr_mode::implied);
        DEX();
        break;
    case 0x88:
        logf(log_level::instr, "DEY");
        current_instruction.set_mnemonic("DEY");
        current_instruction.set_mode(adr_mode::implied);
        DEY();
        break;
    case 0x38:
        logf(log_level::instr, "SEC");
        current_instruction.set_mnemonic("SEC");
        current_instruction.set_mode(adr_mode::implied);
        SEC();
        break;
    case 0x78:
        logf(log_level::instr, "SEI");
        current_instruction.set_mnemonic("SEI");
        current_instruction.set_mode(adr_mode::implied);
        SEI();
        break;
    case 0x18:
        logf(log_level::instr, "CLC");
        current_instruction.set_mnemonic("CLC");
        current_instruction.set_mode(adr_mode::implied);
        CLC();
        break;
    case 0xD8:
        logf(log_level::instr, "CLD");
        current_instruction.set_mnemonic("CLD");
        current_instruction.set_mode(adr_mode::implied);
        CLD();
        break;
    case 0x08:
        logf(log_level::instr, "PHP");
        current_instruction.set_mnemonic("PHP");
        current_instruction.set_mode(adr_mode::implied);
        PHP();
        break;
    case 0x28:
        logf(log_level::instr, "PLP");
        current_instruction.set_mnemonic("PLP");
        current_instruction.set_mode(adr_mode::implied);
        PLP();
        break;
    case 0x00:
        logf(log_level::instr, "BRK");
        current_instruction.set_mnemonic("BRK");
        current_instruction.set_mode(adr_mode::implied);
        BRK();
        break;
    default:
        logf(log_level::error, "Unrecognized instruction %#04x\n", opcode);
        faulted = true;
        break;
    }
    logf(log_level::instr, "\n");
    store.push(current_instruction);
}

uint16_t core6502::zero(uint8_t adr) { return adr; }

uint16_t core6502::zero_x(uint8_t adr) {
    uint8_t mod_adr = adr + x;
    return mod_adr;
}

uint16_t core6502::zero_y(uint8_t adr) {
    uint8_t mod_adr = adr + y;
    return mod_adr;
}

uint16_t core6502::absolute(uint8_t low, uint8_t high) {
    return (high << 8) + low;
}

uint16_t core6502::absolute_x(uint8_t low, uint8_t high) {
    return (high << 8) + low + x;
}

uint16_t core6502::absolute_y(uint8_t low, uint8_t high) {
    return (high << 8) + low + y;
}

uint16_t core6502::indirect_x(uint8_t adr) {
    uint8_t low = bus->read(adr + x);
    uint8_t high = bus->read(adr + x + 1);
    return low + (high << 8);
}

uint16_t core6502::indirect_y(uint8_t adr) {
    uint8_t low = bus->read(adr);
    uint8_t high = bus->read(adr + 1);
    uint16_t raw_adr = low + (high << 8);
    uint16_t mod_adr = raw_adr + y;
    return mod_adr;
}

uint16_t core6502::indirect(uint8_t low_adr, uint8_t high_adr) {
    uint16_t base_adr = (high_adr << 8) + low_adr;
    // If the address passes a page boundary do not increment the high part.
    uint16_t next_adr = (high_adr << 8) + static_cast<uint8_t>(low_adr + 1);
    return bus->read(base_adr) + (bus->read(next_adr) << 8);
}

void core6502::set_zero_flag(uint8_t val) {
    if (!val)
        status |= zero_flag;
    else
        status &= ~zero_flag;
}

void core6502::set_negative_flag(uint8_t val) {
    if (val & (1 << 7))
        status |= negative_flag;
    else
        status &= ~negative_flag;
}

void core6502::set_overflow_flag_add(uint8_t prev, uint8_t res) {
    if (((0x80 & prev) ^ 0x80) & (0x80 & res))
        status |= overflow_flag;
    else
        status &= ~overflow_flag;
}

void core6502::LDA(uint8_t val) {
    set_accumulator(val);
    set_zero_flag(accumulator);
    set_negative_flag(accumulator);
}

void core6502::LDY(uint8_t val) {
    set_y(val);
    set_zero_flag(y);
    set_negative_flag(y);
}

void core6502::LDX(uint8_t val) {
    set_x(val);
    set_zero_flag(x);
    set_negative_flag(x);
}

void core6502::ADC(uint8_t val) {
    uint16_t res = val + accumulator + (status & carry_flag ? 1 : 0);
    set_accumulator(static_cast<uint8_t>(res & 0xFF));
    if (res > UINT8_MAX) {
        status |= carry_flag;
    } else {
        status &= ~carry_flag;
    }
    set_zero_flag(accumulator);
    set_overflow_flag_add(val, accumulator);
    set_negative_flag(accumulator);
}

void core6502::SBC(uint8_t val) {
    uint16_t res = accumulator - !(status & carry_flag) - val;
    set_accumulator(static_cast<uint8_t>(res & 0xFF));
    if (res > UINT8_MAX)
        status &= ~carry_flag;
    else
        status |= carry_flag;
    set_zero_flag(accumulator);
    set_overflow_flag_add(accumulator, static_cast<uint8_t>(res & 0xFF));
    set_negative_flag(accumulator);
}

void core6502::AND(uint8_t val) {
    set_accumulator(accumulator & val);
    set_zero_flag(accumulator);
    set_negative_flag(accumulator);
}

void core6502::ASL() { set_accumulator(ASL(accumulator)); }

void core6502::ASL(uint16_t adr) {
    uint8_t val = bus->read(adr);
    bus->write(adr, ASL(val));
}

uint8_t core6502::ASL(uint8_t val) {
    // Set carry flag
    if (0x80 & val)
        status |= carry_flag;
    else
        status &= ~carry_flag;
    uint8_t res = val << 1;
    set_zero_flag(res);
    return res;
}

void core6502::CMP(uint8_t val) { compare(accumulator, val); }

void core6502::CPX(uint8_t val) { compare(x, val); }

void core6502::CPY(uint8_t val) { compare(y, val); }

void core6502::compare(uint8_t reg, uint8_t val) {
    if (reg >= val)
        status |= carry_flag;
    else
        status &= ~carry_flag;
    if (reg == val)
        status |= zero_flag;
    else
        status &= ~zero_flag;
    // negative (a bit of a guess)
    if (reg < val)
        status |= negative_flag;
    else
        status &= ~negative_flag;
}

void core6502::set_accumulator(uint8_t val) {
    accumulator = val;
    logf(log_level::instr, "\tA=%02x", accumulator);
}

void core6502::set_x(uint8_t val) {
    x = val;
    logf(log_level::instr, "\tx=%02x", x);
}

void core6502::set_y(uint8_t val) {
    y = val;
    logf(log_level::instr, "\ty=%02x", y);
}

void core6502::set_sp(uint8_t val) {
    sp = val;
    logf(log_level::instr, "\tsp=%02x", sp);
}

void core6502::set_pp(uint16_t new_pp) {
    pp = new_pp;
    logf(log_level::instr, "\tpp=%04x", pp);
}

void core6502::BIT(uint8_t val) {
    if (accumulator & val)
        status |= zero_flag;
    else
        status &= ~zero_flag;
    if (0x40 & val)
        status |= overflow_flag;
    else
        status &= ~overflow_flag;
    if (0x80 & val)
        status |= negative_flag;
    else
        status &= ~negative_flag;
}

void core6502::LSR() { set_accumulator(LSR(accumulator)); }

void core6502::LSR(uint16_t adr) {
    uint8_t val = bus->read(adr);
    bus->write(adr, LSR(val));
}

uint8_t core6502::LSR(uint8_t val) {
    if (0x01 & val)
        status |= carry_flag;
    else
        status &= ~carry_flag;
    uint8_t res = val >> 1;
    set_zero_flag(res);
    return res;
}

void core6502::ROL() {
    set_accumulator(ROL(accumulator));
    set_zero_flag(accumulator);
}

void core6502::ROL(uint16_t adr) { bus->write(adr, ROL(bus->read(adr))); }

uint8_t core6502::ROL(uint8_t val) {
    bool carry_set = status & carry_flag;
    status &= ~carry_flag;
    auto bit7 = val & (1 << 7);
    if (bit7)
        status |= carry_flag;
    val <<= 1;
    val += carry_set;
    set_negative_flag(val);
    return val;
}

void core6502::ROR() {
    set_accumulator(ROR(accumulator));
    set_zero_flag(accumulator);
}

void core6502::ROR(uint16_t adr) { bus->write(adr, ROR(bus->read(adr))); }

uint8_t core6502::ROR(uint8_t val) {
    bool carry_set = status & carry_flag;
    status &= ~carry_flag;
    auto bit0 = val & 1;
    if (bit0)
        status |= carry_flag;
    val >>= 1;
    val += carry_set << 7;
    set_negative_flag(val);
    return val;
}

void core6502::ORA(uint8_t val) {
    set_accumulator(accumulator | val);
    set_zero_flag(accumulator);
    set_negative_flag(accumulator);
}

void core6502::EOR(uint8_t val) {
    set_accumulator(accumulator ^ val);
    set_zero_flag(accumulator);
    set_negative_flag(accumulator);
}

void core6502::INC(uint16_t adr) {
    uint8_t val = bus->read(adr);
    val++;
    bus->write(adr, val);
    set_zero_flag(val);
    set_negative_flag(val);
}

void core6502::INX() {
    set_x(x + 1);
    set_zero_flag(x);
    set_negative_flag(x);
}

void core6502::INY() {
    set_y(y + 1);
    set_zero_flag(y);
    set_negative_flag(y);
}

void core6502::PHA() { push(accumulator); }

void core6502::PLA() {
    set_accumulator(pop());
    set_zero_flag(accumulator);
    set_negative_flag(accumulator);
}

void core6502::PHP() { push(status); }

void core6502::PLP() { status = pop(); }

void core6502::TAX() {
    set_x(accumulator);
    set_zero_flag(x);
    set_negative_flag(x);
}

void core6502::TXA() {
    set_accumulator(x);
    set_zero_flag(accumulator);
    set_negative_flag(accumulator);
}

void core6502::TAY() {
    set_y(accumulator);
    set_zero_flag(y);
    set_negative_flag(y);
}

void core6502::TYA() {
    set_accumulator(y);
    set_zero_flag(accumulator);
    set_negative_flag(accumulator);
}

void core6502::TXS() { set_sp(x); }

void core6502::DEC(uint16_t adr) {
    uint8_t val = bus->read(adr);
    val--;
    bus->write(adr, val);
    set_zero_flag(val);
    set_negative_flag(val);
}

void core6502::DEX() {
    set_x(x - 1);
    set_zero_flag(x);
    set_negative_flag(x);
}

void core6502::DEY() {
    set_y(y - 1);
    set_zero_flag(y);
    set_negative_flag(y);
}

void core6502::SEC() { status |= carry_flag; }

void core6502::SEI() { status |= interrupt_disable_flag; }

void core6502::CLC() { status &= ~carry_flag; }

void core6502::CLD() { status &= ~decimal_flag; }

void core6502::BEQ(uint8_t val) {
    if (status & zero_flag) {
        int8_t sval = (int8_t)val;
        set_pp(pp + sval);
    }
}

void core6502::BMI(uint8_t val) {
    if (status & negative_flag) {
        int8_t sval = (int8_t)val;
        set_pp(pp + sval);
    }
}

void core6502::BNE(uint8_t val) {
    if (!(status & zero_flag)) {
        int8_t sval = (int8_t)val;
        set_pp(pp + sval);
    }
}

void core6502::BCS(uint8_t val) {
    if (status & carry_flag) {
        int8_t sval = (int8_t)val;
        set_pp(pp + sval);
    }
}

void core6502::BCC(uint8_t val) {
    if (!(status & carry_flag)) {
        int8_t sval = (int8_t)val;
        set_pp(pp + sval);
    }
}

void core6502::BPL(uint8_t val) {
    if (!(status & negative_flag)) {
        int8_t sval = (int8_t)val;
        set_pp(pp + sval);
    }
}

void core6502::BVC(uint8_t val) {
    if (!(status & overflow_flag)) {
        int8_t sval = (int8_t)val;
        set_pp(pp + sval);
    }
}

void core6502::pushpp() {
    uint16_t pushval = pp - 1;
    uint8_t pp_low = (uint8_t)pushval;
    uint8_t pp_high = (uint8_t)(pushval >> 8);
    push(pp_low);
    push(pp_high);
}

void core6502::JSR(uint16_t adr) {
    pushpp();
    set_pp(adr);
}

void core6502::JMP(uint16_t adr) { set_pp(adr); }

void core6502::RTI() {
    status = pop();
    uint8_t high = pop();
    uint8_t low = pop();
    uint16_t adr = (high << 8) + low;
    set_pp(adr + 1);
}

void core6502::RTS() {
    uint8_t high = pop();
    uint8_t low = pop();
    uint16_t adr = (high << 8) + low;
    set_pp(adr + 1);
}

void core6502::BRK() {
    pushpp();
    push(status |= ((1 << 4) + (1 << 5)));
    uint8_t low = bus->read(0xFFFE);
    uint8_t high = bus->read(0xFFFF);
    uint16_t adr = low + (high << 8);
    set_pp(adr);
}

void core6502::STA(uint16_t adr) { bus->write(adr, accumulator); }

void core6502::STX(uint16_t adr) { bus->write(adr, x); }

void core6502::STY(uint16_t adr) { bus->write(adr, y); }
