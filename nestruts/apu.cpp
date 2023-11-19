#include <span>

#include "apu.h"
#include "log.h"

namespace {
constexpr uint8_t frame_counter_mode_bit = 1 << 7;
constexpr uint8_t inhibit_irq_bit = 1 << 6;
} // namespace

void audio_processing_unit::pulse::play(std::span<std::int16_t> audio_buffer,
                                        int sample_rate_hz) {
    // Multiply set volume by arbitrary multiplier
    int volume = volume_envelope() * 256;
    int period_ticks = ((0x7 & reg_length_timer) << 8) + reg_timer_low + 1;
    float frequency = 111860.8f / period_ticks;
    if (frequency > 2000)
        volume = 0; // HACK
    float period_s = 1 / frequency;
    // Generate a square wave
    float period_samples = sample_rate_hz * period_s;
    for (auto &val : audio_buffer) {
        if (counter_pulse_1 >= period_samples) {
            counter_pulse_1 = 0;
        }
        val += counter_pulse_1++ > period_samples / 2 ? volume : -volume;
    }
}

int audio_processing_unit::pulse::volume_envelope() {
    return (reg_dlcn & 0x15);
}

void audio_processing_unit::cycle() { --cycles_til_irq; }

void audio_processing_unit::set_frame_counter(uint8_t val) {
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
