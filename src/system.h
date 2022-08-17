#pragma once

#include <string>

#include "cpu.h"
#include "ppu.h"
#include "interrupts.h"
#include "timer.h"
#include <list>
class System
{
public:
    Interrupts interrupts;
    Timer timer;
    Cartridge cart;
    Bus bus;
    Cpu cpu;
    Ppu ppu;

    std::string serial_output;

    System(const std::string &cartridge_filename);

    void tick();
};

