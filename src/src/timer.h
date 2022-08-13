#pragma once

#include "interrupts.h"

struct Timer
{
    Interrupts &interrupts;

    // 0xFF04 DIV - Divider Register
    std::uint16_t divider = 0xAC00;
    // 0xFF05 TIMA - Timer counter
    std::uint8_t counter = 0;
    // 0xFF06 - TMA - Timer Modulo
    std::uint8_t modulo = 0;
    // 0xFF07 - TAC - Timer Control
    std::uint8_t control = 0;

    std::uint8_t read(std::uint16_t address);
    void write(std::uint16_t address, std::uint8_t value);

    void run_once();
};
