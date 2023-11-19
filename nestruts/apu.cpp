#include <span>

#include "apu.h"
#include "log.h"

namespace {
constexpr uint8_t frame_counter_mode_bit = 1 << 7;
constexpr uint8_t inhibit_irq_bit = 1 << 6;
} // namespace

void audio_processing_unit::pulse::play(std::span<std::int16_t> audio_buffer,
                                        int sample_rate_hz) {
    // FIXME: Doesn't work for some reaons, comment out to get some sound :-)
    // if (!enabled) return;
    // Multiply set volume by arbitrary multiplier
    int volume = volume_envelope() * 256;
    int period_ticks = ((0x7 & reg_length_timer) << 8) + reg_timer_low + 1;
    float frequency = 111860.8f / period_ticks;
    float period_s = 1 / frequency;
    // Generate a square wave
    float period_samples = sample_rate_hz * period_s;
    for (auto &val : audio_buffer) {
        if (length_counter == 0)
            break;
        if (counter_pulse_1 >= period_samples) {
            counter_pulse_1 = 0;
        }
        val += counter_pulse_1++ > period_samples / 2 ? volume : -volume;
        if (!length_counter_halt()) {
            // FIXME: Support also 4-step sequence
            // 5-step sequence length counter stepped at 96 Hz.
            if (!--samples_until_length_counter) {
                log(log_level::error, "Decreasing length counter\n");
                samples_until_length_counter = sample_rate_hz * 60 / 96;
                --length_counter;
            }
        }
    }
}

bool audio_processing_unit::pulse::length_counter_halt() {
    return reg_dlcn & 1 << 5;
}

int audio_processing_unit::pulse::volume_envelope() {
    return (reg_dlcn & 0x15);
}

void audio_processing_unit::pulse::load_length_counter() {
    // FIXME: Don't assume enabled, check
    int val = reg_length_timer >> 3;
    std::array<int, 32> length_lut{
        10, 254, 20, 2,  40, 4,  80, 6,  160, 8,  60, 10, 14, 12, 26, 14,
        12, 16,  24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30};
    length_counter = length_lut.at(val);
    log(log_level::debug, "\tapu loaded length counter {}", length_counter);
}

void audio_processing_unit::cycle() { --cycles_til_irq; }

void audio_processing_unit::set_frame_counter(uint8_t val) {
    log(log_level::debug, "\twrite APU frame counter");
    frame_counter_mode = val & frame_counter_mode_bit;
    logf(log_level::debug, " frame_counter_mode=%i", frame_counter_mode);
    inhibit_irq = val & inhibit_irq_bit;
    logf(log_level::debug, " inhibit_irq=%i", inhibit_irq);
}

void audio_processing_unit::play_audio() {
    // Not an exact emulation of the APU. Instead create the sound that is
    // expected.
    for (auto &v : audio_buffer)
        v = 0;

    int required_samples{audio.required_samples()};
    int sample_rate_hz{audio.sample_rate_hz()};

    auto section = std::span(audio_buffer.begin(), required_samples);

    pulse1.play(section, sample_rate_hz);
    pulse2.play(section, sample_rate_hz);

    // Play triangle
    // Play noise
    // Play DMC
    audio.queue(section);
}

void audio_processing_unit::write_status(uint8_t val) {
    // FIXME: For some reason this is never written to.
    log(log_level::debug, "\twriting APU status 2: {} 1: {}\n", val & 1, val & 1 << 1);
    pulse1.enable(val & 1);
    pulse2.enable(val & 1 << 1);
}

uint8_t audio_processing_unit::read_status() {
    log(log_level::debug, "\tread APU status\n");
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
