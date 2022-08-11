#pragma once

#include "cartridge.h"
#include "interrupts.h"
#include "ppu.h"

#include <cstdint>
#include <string>
#include <vector>

struct Timer {

    // 0xFF04 DIV - Divider Register
    std::uint16_t divider = 0xAC00;
    // 0xFF05 TIMA - Timer counter
    std::uint8_t counter = 0;
    // 0xFF06 - TMA - Timer Modulo
    std::uint8_t modulo = 0;
    // 0xFF07 - TAC - Timer Control
    std::uint8_t control = 0;
};

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

    Ppu &ppu;

    // 0x0000 - 0x3FFF : ROM Bank 0
    // 0x4000 - 0x7FFF : ROM Bank 1 - Switchable
    Cartridge &cart;
    // 0x8000 - 0x97FF : CHR RAM
    std::uint8_t video_ram[0x2000];
    // 0x9800 - 0x9BFF : BG Map 1
    // 0x9C00 - 0x9FFF : BG Map 2
    // 0xA000 - 0xBFFF : Cartridge RAM
    // 0xC000 - 0xCFFF : RAM Bank 0
    std::uint8_t work_ram1[0xCFFF-0xC000] = {0};
    // 0xD000 - 0xDFFF : RAM Bank 1-7 - switchable - Color only
    std::uint8_t work_ram2[0xDFFF-0xD000] = {0};
    // 0xE000 - 0xFDFF : Reserved - Echo RAM
    std::uint8_t *work_ram = work_ram1;

    // 0xFEA0 - 0xFEFF : Reserved - Unusable

    // 0xFF80 - 0xFFFE : Zero Page
    std::uint8_t high_ram[0xFFFE-0xFF80] = {0};



    // 0xFF00 - 0xFF7F : I/O Registers
    // 0xFF00 - P1/JOYP - Joypad (R/W)
//    The eight Game Boy action/direction buttons are arranged as a 2x4 matrix.
//    Select either action or direction buttons by writing to this register, then read out the bits 0-3.
//    Bit 7 - Not used
//    Bit 6 - Not used
//    Bit 5 - P15 Select Action buttons    (0=Select)
//    Bit 4 - P14 Select Direction buttons (0=Select)
//    Bit 3 - P13 Input: Down  or Start    (0=Pressed) (Read Only)
//    Bit 2 - P12 Input: Up    or Select   (0=Pressed) (Read Only)
//    Bit 1 - P11 Input: Left  or B        (0=Pressed) (Read Only)
//    Bit 0 - P10 Input: Right or A        (0=Pressed) (Read Only)
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

    Timer timer;
    void run_timer_once();

    Interrupts interrupts;
};

