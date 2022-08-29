#pragma once

#include "cartridge.h"
#include "interrupts.h"
#include "timer.h"
#include "ppu.h"

struct Bus
{
    std::uint8_t read(std::uint16_t address);
    void write(std::uint16_t address, std::uint8_t value);

    std::uint16_t read16(std::uint16_t address)
    {
        std::uint8_t lsb = read(address + 0);
        std::uint8_t msb = read(address + 1);
        return lsb | msb << 8;
    }
    void write16(std::uint16_t address, std::uint16_t value)
    {
        write(address + 0,  value & 0x00ff);
        write(address + 1, (value & 0xff00) >> 8);
    }

    Interrupts &interrupts;
    Timer &timer;
    Ppu &ppu;

    // 0x0000 - 0x3FFF : ROM Bank 0
    // 0x4000 - 0x7FFF : ROM Bank 1 - Switchable
    Cartridge &cart;

    // 0xC000 - 0xCFFF : RAM Bank 0
    std::uint8_t work_ram1[0x1000] = {0};

    // 0xD000 - 0xDFFF : RAM Bank 1-7 - switchable - Color only
    std::uint8_t work_ram2[0x1000] = {0};

    // 0xFF80 - 0xFFFE : High RAM (HRAM)
    std::uint8_t high_ram[0x80] = {0};

    // 0xFF00 - P1/JOYP - Joypad (R/W)
    struct JoypadState
    {
        bool up = false;
        bool down = false;
        bool left = false;
        bool right = false;
        bool start = false;
        bool select = false;
        bool a = false;
        bool b = false;

        std::uint8_t query = 0;
    } p1_joypad;



};

