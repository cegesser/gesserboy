#pragma once

#include <cstdint>

struct Interrupts
{
    enum Type {
        VBLANK	= 1 << 0,
        LCDSTAT	= 1 << 1,
        TIMER	= 1 << 2,
        SERIAL	= 1 << 3,
        JOYPAD	= 1 << 4,
    };

    std::uint8_t trigger_register = 0; //0xFF0F
    std::uint8_t enable_register = 0; //0xFFFF

    void trigger_interrupt(Type interrupt);
    int active_interrupt_address();
};

