#include "timer.h"

uint8_t Timer::read(uint16_t address)
{
    if (address == 0xFF04) return divider;
    if (address == 0xFF05) return counter;
    if (address == 0xFF06) return modulo;
    if (address == 0xFF07) return control;

    return 0xFF;
}

void Timer::write(uint16_t address, uint8_t value)
{
    if (address == 0xFF04) divider = 0;
    if (address == 0xFF05) counter = value;
    if (address == 0xFF06) modulo  = value;
    if (address == 0xFF07) control = value;
}

void Timer::run_once()
{
    ++divider;

    const bool timer_enable = control & (1 << 2);
    if ( ! timer_enable )
    {
        return;
    }

    const auto input_clock_select = control & 0b11;

    bool timer_update = input_clock_select == 0 ? (divider % 1024) == 0
                      : input_clock_select == 3 ? (divider % 256) == 0
                      : input_clock_select == 2 ? (divider % 64) == 0
                      : input_clock_select == 1 ? (divider % 16) == 0
                                                : false;

    if ( ! timer_update )
    {
        return;
    }

    ++counter;

    if (counter == 0xFF)
    {
        counter = modulo;

        interrupts.trigger_interrupt(Interrupts::TIMER);
    }
}
