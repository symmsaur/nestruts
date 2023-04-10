#pragma once
#include <cstdint>

class audio_processing_unit final {
  public:
    void cycle();
    void set_frame_counter(uint8_t val);

    uint8_t read_status();

    // Should interrupt trigger?
    bool IRQ();

    void pulse1_dlcn(uint8_t val);
    void pulse1_sweep(uint8_t val);
    void pulse1_timer_low(uint8_t val);
    void pulse1_length_timer(uint8_t val);

  private:
    bool frame_counter_mode = false;
    bool inhibit_irq = false;
    int cycles_til_irq = 5000; // Should be 60 Hz (is not!).

    uint8_t reg_pulse1_dlcn{};
    uint8_t reg_pulse1_sweep{};
    uint8_t reg_pulse1_timer_low{};
    uint8_t reg_pulse1_length_timer{};
};
