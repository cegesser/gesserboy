#pragma once

#include <string>

#include "cpu.h"
#include "ppu.h"

class System
{
public:
    Cartridge cart;
    Bus bus;
    Cpu cpu;
    Ppu ppu;

    std::vector<std::string> log;

    std::string serial_output;

    System(const std::string &cartridge_filename);

    void tick();
};

