#include <catch2/catch_test_macros.hpp>
#include <memory>
#include "nestruts/core6502.h"
#include "nestruts/mem.h"

TEST_CASE("LDA Immediate", "[instruction]") {
	auto m = std::make_unique<memory_bus>(nullptr, nullptr);
	m->write(0x0000, 0xA9); // LDA immediate
	m->write(0x0001, 0x11); // value
    auto cpu = std::make_unique<core6502>(std::move(m), []{return false; }, []{return false;});
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
    auto cpu = std::make_unique<core6502>(std::move(m), []{return false; }, []{return false;});
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
    auto cpu = std::make_unique<core6502>(std::move(m), []{return false; }, []{return false;});
	cpu->setpp(0x0000);
	cpu->cycle();
	cpu->cycle();
    REQUIRE(cpu->get_acc() == 0x08 + 0x09);
}
