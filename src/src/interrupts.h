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

};

