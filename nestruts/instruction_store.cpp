#include "instruction_store.h"

#include "fmt/core.h"
#include <fstream>

instruction_store::~instruction_store() {
    FILE *file{fopen("disasm_dump", "w")};
    for (auto &instr : m_instructions) {
        auto const &instruction = instr.second;
        fmt::print(file, "${:04x}: {}", instruction.pp(),
                   instruction.mnemonic());
        switch (instruction.mode()) {
        case adr_mode::implied:
            // Print nothing
            break;
        case adr_mode::accumulator:
            fmt::print(file, " A");
            break;
        case adr_mode::immediate:
            fmt::print(file, " #{:02x}", instruction.argument());
            break;
        case adr_mode::absolute:
            fmt::print(file, " ${:04x}", instruction.argument());
            break;
        case adr_mode::x_indexed_absolute:
            fmt::print(file, " ${:04x},X", instruction.argument());
            break;
        case adr_mode::y_indexed_absolute:
            fmt::print(file, " ${:04x},Y", instruction.argument());
            break;
        case adr_mode::absolute_indirect:
            fmt::print(file, " (${:04x})", instruction.argument());
            break;
        case adr_mode::zero_page:
            fmt::print(file, " ${:02x}", instruction.argument());
            break;
        case adr_mode::x_indexed_zero_page:
            fmt::print(file, " ${:02x},X", instruction.argument());
            break;
        case adr_mode::y_indexed_zero_page:
            fmt::print(file, " ${:02x},Y", instruction.argument());
            break;
        case adr_mode::x_indexed_zero_page_indirect:
            fmt::print(file, " (${:02x},X)", instruction.argument());
            break;
        case adr_mode::zero_page_indirect_y_indexed:
            fmt::print(file, " (${:02x}),Y", instruction.argument());
            break;
        case adr_mode::relative:
            fmt::print(file, " *{:02x}",
                       static_cast<int8_t>(instruction.argument()));
            break;
        }
        // Make it easier to tell subroutines apart
        if (instruction.mnemonic() == "RTS" ||
            instruction.mnemonic() == "RTI" || instruction.mnemonic() == "JMP")
            fmt::print(file, "\n");
        fmt::print(file, "\n");
    }
    fclose(file);
}
