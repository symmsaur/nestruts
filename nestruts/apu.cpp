#include "apu.h"
#include "log.h"

namespace {
constexpr uint8_t frame_counter_mode_bit = 1 << 7;
constexpr uint8_t inhibit_irq_bit = 1 << 6;
} // namespace

void audio_processing_unit::cycle() { --cycles_til_irq; }

void audio_processing_unit::set_frame_counter(uint8_t val) {
    frame_counter_mode = val & frame_counter_mode_bit;
    logf(log_level::debug, " frame_counter_mode=%i", frame_counter_mode);
    inhibit_irq = val & inhibit_irq_bit;
    logf(log_level::debug, " inhibit_irq=%i", inhibit_irq);
}

uint8_t audio_processing_unit::read_status() {
    // Only implemented frame interrupt.
    uint8_t res{};
    if (cycles_til_irq) {
        // Set frame interrupt bit
        res |= 1 << 6;
    }
    // ... more bits ...
    cycles_til_irq = 5000;
    return res;
}

bool audio_processing_unit::IRQ() {
    if (!inhibit_irq && cycles_til_irq <= 0) {
        return true;
    } else {
        return false;
    }
}

void audio_processing_unit::pulse1_dlcn(uint8_t val) {
    log(log_level::debug, "Set pulse1_dlcn\n");
    reg_pulse1_dlcn = val;
}

void audio_processing_unit::pulse1_sweep(uint8_t val) {
    log(log_level::debug, "Set pulse1_sweep\n");
    reg_pulse1_sweep = val;
}

void audio_processing_unit::pulse1_timer_low(uint8_t val) {
    log(log_level::debug, "Set pulse1_timer_low\n");
    reg_pulse1_timer_low = val;
}

void audio_processing_unit::pulse1_length_timer(uint8_t val) {
    log(log_level::debug, "Set pulse1_length_timer\n");
    reg_pulse1_length_timer = val;
}
