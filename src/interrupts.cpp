#include "interrupts.h"

void Interrupts::trigger_interrupt(Type interrupt)
{
    trigger_register |= interrupt;
}

#include <iostream>

int Interrupts::active_interrupt_address()
{
    const auto interrupts_engaged = enable_register & trigger_register;

    if ( ! interrupts_engaged ) {
        return 0;
    }

    static constexpr struct {
        Interrupts::Type interrupt;
        std::uint16_t address;
    } handlers[] = {
        { Interrupts::VBLANK,  0x40 },
        { Interrupts::LCDSTAT, 0x48 },
        { Interrupts::TIMER,   0x50 },
        { Interrupts::SERIAL,  0x58 },
        { Interrupts::JOYPAD,  0x60 }
    };

    for (const auto &handler : handlers)
    {
        auto flag = int(handler.interrupt);
        if (interrupts_engaged & flag) {

            trigger_register &= ~flag;

            return handler.address;
        }
    }

    return 0;
}
