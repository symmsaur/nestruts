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
    cpu->cycle();
    REQUIRE(expected_state == post_state);
    REQUIRE(cpu->get_acc() == 0x01);
}

TEST_CASE("AND Immediate", "[instruction]") {
    auto m = std::make_unique<memory_bus>(nullptr, nullptr);
    m->write(0x0000, 0xA9); // LDA immediate
    m->write(0x0001, 0x11); // value
    m->write(0x0002, 0x29); // AND Immediate
    m->write(0x0003, 0x22); // value
    auto cpu = std::make_unique<core6502>(
        std::move(m), [] { return false; }, [] { return false; });
    cpu->setpp(0x0000);
    cpu->cycle(); // LDA 0x11
    auto expected_state = cpu->dump_state();
    expected_state.pp += 2;
    expected_state.accumulator = 0x00;
    // TODO: Expose these flag definitions
    expected_state.status |= 1 << 1; // Zero flag should be set
    cpu->cycle();                    // AND 0x22
    auto post_state = cpu->dump_state();
    REQUIRE(expected_state == post_state);
    REQUIRE(cpu->get_acc() == 0x00);
}
