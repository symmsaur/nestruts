#include "nestruts/core6502.h"
#include "nestruts/mem.h"
#include <catch2/catch_test_macros.hpp>
#include <memory>

TEST_CASE("LDA Immediate", "[instruction]") {
    auto m = std::make_unique<memory_bus>(nullptr, nullptr);
    m->write(0x0000, 0xA9); // LDA immediate
    m->write(0x0001, 0x11); // value
    auto cpu = std::make_unique<core6502>(
        std::move(m), [] { return false; }, [] { return false; });
    cpu->setpp(0x0000);
    cpu->cycle();
    REQUIRE(cpu->get_acc() == 0x11);
}

TEST_CASE("LDA Absolute", "[instruction]") {
    auto m = std::make_unique<memory_bus>(nullptr, nullptr);
    m->write(0x0000, 0xAD); // LDA Absolute
    m->write(0x0001, 0x34); // addr low
    m->write(0x0002, 0x12); // addr high
    m->write(0x1234, 0xAB); // value to read
    auto cpu = std::make_unique<core6502>(
        std::move(m), [] { return false; }, [] { return false; });
    cpu->setpp(0x0000);
    cpu->cycle();
    REQUIRE(cpu->get_acc() == 0xAB);
}

TEST_CASE("LDA X-Indexed Absolute", "[instruction]") {
    auto m = std::make_unique<memory_bus>(nullptr, nullptr);
    m->write(0x0000, 0xA2); // LDX immediate
    m->write(0x0001, 0x10); // value
    m->write(0x0002, 0xBD); // LDA X-Indexed Absolute
    m->write(0x0003, 0x34); // addr low
    m->write(0x0004, 0x12); // addr high
    m->write(0x1244, 0xCD); // value to read
    auto cpu = std::make_unique<core6502>(
        std::move(m), [] { return false; }, [] { return false; });
    cpu->setpp(0x0000);
    cpu->cycle(); // LDX 0x10
    cpu->cycle(); // LDA
    REQUIRE(cpu->get_acc() == 0xCD);
}

TEST_CASE("LDA Y-Indexed Absolute", "[instruction]") {
    auto m = std::make_unique<memory_bus>(nullptr, nullptr);
    m->write(0x0000, 0xA0); // LDY immediate
    m->write(0x0001, 0x10); // value
    m->write(0x0002, 0xB9); // LDA Y-Indexed Absolute
    m->write(0x0003, 0x34); // addr low
    m->write(0x0004, 0x12); // addr high
    m->write(0x1244, 0xCE); // value to read
    auto cpu = std::make_unique<core6502>(
        std::move(m), [] { return false; }, [] { return false; });
    cpu->setpp(0x0000);
    cpu->cycle(); // LDX 0x10
    cpu->cycle(); // LDA
    REQUIRE(cpu->get_acc() == 0xCE);
}

TEST_CASE("LDA Zero Page", "[instruction]") {
    auto m = std::make_unique<memory_bus>(nullptr, nullptr);
    m->write(0x0000, 0xA5); // LDA Zero Page
    m->write(0x0001, 0x44); // offset
    m->write(0x0044, 0xBA); // value to read
    auto cpu = std::make_unique<core6502>(
        std::move(m), [] { return false; }, [] { return false; });
    cpu->setpp(0x0000);
    cpu->cycle(); // LDA
    REQUIRE(cpu->get_acc() == 0xBA);
}

TEST_CASE("LDA X-Indexed Zero Page", "[instruction]") {
    auto m = std::make_unique<memory_bus>(nullptr, nullptr);
    m->write(0x0000, 0xA2); // LDX immediate
    m->write(0x0001, 0x11); // value
    m->write(0x0002, 0xB5); // LDA X-Indexed Zero Page
    m->write(0x0003, 0x44); // offset
    m->write(0x0055, 0xCB); // value to read
    auto cpu = std::make_unique<core6502>(
        std::move(m), [] { return false; }, [] { return false; });
    cpu->setpp(0x0000);
    cpu->cycle(); // LDX
    cpu->cycle(); // LDA
    REQUIRE(cpu->get_acc() == 0xCB);
}

TEST_CASE("LDA X-Indexed Zero Page Indirect", "[instruction]") {
    auto m = std::make_unique<memory_bus>(nullptr, nullptr);
    m->write(0x0000, 0xA2); // LDX immediate
    m->write(0x0001, 0x12); // value
    m->write(0x0002, 0xA1); // LDA X-Indexed Zero Page Indirect
    m->write(0x0003, 0x31); // offset
    m->write(0x0043, 0xAB); // address low
    m->write(0x0044, 0x12); // address high
    m->write(0x12AB, 0x17); // value to read
    auto cpu = std::make_unique<core6502>(
        std::move(m), [] { return false; }, [] { return false; });
    cpu->setpp(0x0000);
    cpu->cycle(); // LDX
    cpu->cycle(); // LDA
    REQUIRE(cpu->get_acc() == 0x17);
}

TEST_CASE("LDA Zero Page Indirect Y-Indexed", "[instruction]") {
    auto m = std::make_unique<memory_bus>(nullptr, nullptr);
    m->write(0x0000, 0xA0); // LDY immediate
    m->write(0x0001, 0x02); // value
    m->write(0x0002, 0xB1); // LDA Zero Page Indirect Y-Indexed
    m->write(0x0003, 0x20); // Zero page address
    m->write(0x0020, 0xFF); // addr low (added with Y)
    m->write(0x0021, 0x11); // addr high
    m->write(0x1201, 0x21); // value to read
    auto cpu = std::make_unique<core6502>(
        std::move(m), [] { return false; }, [] { return false; });
    cpu->setpp(0x0000);
    cpu->cycle(); // LDY
    cpu->cycle(); // LDA
    REQUIRE(cpu->get_acc() == 0x21);
}

TEST_CASE("LDX Y-Indexed Zero Page", "[instruction]") {
    auto m = std::make_unique<memory_bus>(nullptr, nullptr);
    m->write(0x0000, 0xA0); // LDY immediate
    m->write(0x0001, 0x10); // value
    m->write(0x0002, 0xB6); // LDX Y-Indexed Zero Page
    m->write(0x0003, 0x31); // offset
    m->write(0x0041, 0x19); // value to read
    auto cpu = std::make_unique<core6502>(
        std::move(m), [] { return false; }, [] { return false; });
    cpu->setpp(0x0000);
    cpu->cycle(); // LDY
    cpu->cycle(); // LDX
    REQUIRE(cpu->get_x() == 0x19);
}

TEST_CASE("ADC Immediate", "[instruction]") {
    auto m = std::make_unique<memory_bus>(nullptr, nullptr);
    m->write(0x0000, 0xA9); // LDA immediate
    m->write(0x0001, 0x11); // value
    m->write(0x0002, 0x69); // ADC immediate
    m->write(0x0003, 0x22); // value
    auto cpu = std::make_unique<core6502>(
        std::move(m), [] { return false; }, [] { return false; });
    cpu->setpp(0x0000);
    cpu->cycle();
    cpu->cycle();
    REQUIRE(cpu->get_acc() == 0x11 + 0x22);
}

TEST_CASE("ADC Absolute", "[instruction]") {
    auto m = std::make_unique<memory_bus>(nullptr, nullptr);
    m->write(0x0000, 0xA9); // LDA immediate
    m->write(0x0001, 0x08); // value
    m->write(0x0002, 0x6D); // ADC absolute
    m->write(0x0003, 0x34); // low adr
    m->write(0x0004, 0x12); // high adr
    m->write(0x1234, 0x09); // value
    auto cpu = std::make_unique<core6502>(
        std::move(m), [] { return false; }, [] { return false; });
    cpu->setpp(0x0000);
    cpu->cycle();
    cpu->cycle();
    REQUIRE(cpu->get_acc() == 0x08 + 0x09);
}

TEST_CASE("PHP", "[instruction]") {
    auto m = std::make_unique<memory_bus>(nullptr, nullptr);
    m->write(0x0000, 0x38); // SEC
    m->write(0x0001, 0x08); // PHP
    m->write(0x0002, 0x68); // PLA
    auto cpu = std::make_unique<core6502>(
        std::move(m), [] { return false; }, [] { return false; });
    cpu->setpp(0x0000);
    cpu->cycle();
    auto expected_state = cpu->dump_state();
    expected_state.pp++;
    expected_state.sp--;
    cpu->cycle();
    auto post_state = cpu->dump_state();
    REQUIRE(expected_state == post_state);
    cpu->cycle();
    REQUIRE(cpu->get_acc() == 0x01);
}

TEST_CASE("AND Immediate", "[instruction]") {
    auto m = std::make_unique<memory_bus>(nullptr, nullptr);
    m->write(0x0000, 0xA9); // LDA immediate
    m->write(0x0001, 0x11); // value
    m->write(0x0002, 0x29); // AND Immediate
    m->write(0x0003, 0x22); // value
    m->write(0x0004, 0xA9); // LDA immediate
    m->write(0x0005, 0x81); // value
    m->write(0x0006, 0x29); // AND Immediate
    m->write(0x0007, 0x82); // value
    auto cpu = std::make_unique<core6502>(
        std::move(m), [] { return false; }, [] { return false; });
    cpu->setpp(0x0000);
    cpu->cycle(); // LDA 0x11
    {
        auto expected_state = cpu->dump_state();
        expected_state.pp += 2;
        expected_state.accumulator = 0x00;
        // TODO: Expose these flag definitions
        expected_state.status |= 1 << 1; // Zero flag should be set
        cpu->cycle();                    // AND 0x22
        auto post_state = cpu->dump_state();
        REQUIRE(expected_state == post_state);
    }
    cpu->cycle(); // LDA 0x81
    {
        auto expected_state = cpu->dump_state();
        expected_state.pp += 2;
        expected_state.accumulator = 0x80;
        expected_state.status |= 1 << 7; // Negative flag should be set
        cpu->cycle();                    // AND 0x82
        auto post_state = cpu->dump_state();
        REQUIRE(expected_state == post_state);
    }
}
