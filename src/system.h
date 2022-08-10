#pragma once

#include <string>

#include "cpu.h"
#include "ppu.h"
#include <list>
class System
{
public:
    Cartridge cart;
    Bus bus;
    Cpu cpu;
    Ppu ppu;

    std::list<std::string> log;

    std::string serial_output;

    System(const std::string &cartridge_filename);

    void tick();
};

