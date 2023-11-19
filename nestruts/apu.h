#pragma once
#include <cstdint>
#include <span>

#include "audio.h"

class audio_processing_unit final {
  public:
    class pulse final {
        // FIXME: Pulse channels are missing envelope, sweep, duty cycle.
      public:
        void dlcn(uint8_t val) { reg_dlcn = val; }
        void sweep(uint8_t val) { reg_sweep = val; }
        void timer_low(uint8_t val) { reg_timer_low = val; }
        void length_timer(uint8_t val) { reg_length_timer = val; }
        void play(std::span<std::int16_t> audio_buffer, int sample_rate_hz);

      private:
        // duty
        // infinite
        // envelope disable
        int volume_envelope();

        int counter_pulse_1{};
        uint8_t reg_dlcn{};
        uint8_t reg_sweep{};
        uint8_t reg_timer_low{};
        uint8_t reg_length_timer{};
    };

    void cycle();
    void set_frame_counter(uint8_t val);
    void play_audio();

    uint8_t read_status();

    // Should interrupt trigger?
    bool IRQ();

    // Any reason I should hide these?
    pulse pulse1{};
    pulse pulse2{};

  private:
    audio_backend audio{};

    std::array<std::int16_t, 2048> audio_buffer{};

    bool frame_counter_mode = false;
    bool inhibit_irq = false;
    int cycles_til_irq = 5000; // Should be 60 Hz (is not!).
};
