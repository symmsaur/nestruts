#include "audio_processing_unit.h"
#include "log.h"

namespace {
	constexpr uint8_t frame_counter_mode_bit = 1 << 7;
	constexpr uint8_t inhibit_irq_bit = 1 << 6;
}

void audio_processing_unit::cycle()
{
	--cycles_til_irq;
}

void audio_processing_unit::set_frame_counter(uint8_t val)
{
	frame_counter_mode = val & frame_counter_mode_bit;
	log(log_level::debug, " frame_counter_mode=%i", frame_counter_mode);
	inhibit_irq = val & inhibit_irq_bit;
	log(log_level::debug, " inhibit_irq=%i", inhibit_irq);
}

uint8_t audio_processing_unit::read_status()
{
	// Only implemented frame interrupt.
	uint8_t res{};
	if (cycles_til_irq)
	{
		// Set frame interrupt bit
		res |= 1 << 6;
	}
	// ... more bits ...
	cycles_til_irq = 5000;
	return res;
}

bool audio_processing_unit::IRQ()
{
	if (!inhibit_irq && cycles_til_irq <= 0)
	{
		return true;
	}
	else
	{
		return false;
	}

}
