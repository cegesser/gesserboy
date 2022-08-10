#pragma once

#include "ppu.h"

#include <cstdint>
#include <string>
#include <vector>

struct CartridgeHeader{
    std::uint8_t entry_point[4];      //0100-0103
    std::uint8_t nintendo_logo[0x30]; //0104-0133

    char title[16];                   //0134-0143
                                      //013F-0142 - Manufacturer Code
                                      //0143 CGB Flag
    std::uint16_t new_licensee_code;   //0144-0145 - New Licensee Code
    std::uint8_t sgb_flag;            //0146 SGB Flag: $00: (Normal GB/CGB) $03: SuperGB functions
    std::uint8_t cartridge_type;      //0147 Cartridge Type
    std::uint8_t rom_size;            //0148 ROM Size
    std::uint8_t ram_size;            //0149 RAM Size
    std::uint8_t destination_code;    //014A Destination Code $00: Japanese $01: Non-Japanese
    std::uint8_t old_licensee_code;   //014B Old Licensee Code
    std::uint8_t mask_version;        //014C Mask ROM Version number
    std::uint8_t header_checksum;     //014D Header Checksum
    std::uint16_t global_checksum;    //014E-014F Global Checksum
};

const char *cartridge_type(const CartridgeHeader *header);

void print_header(const CartridgeHeader *header);

struct Cartridge
{
    std::vector<std::uint8_t> rom_data;
    CartridgeHeader *header;

    int selected_rom_bank = 0;
    uint8_t *rom_bank_ptr = nullptr;

    bool ram_enabled = false;
    bool ram_banking_mode = false;
    int selected_ram_bank = 0;
    std::vector<std::uint8_t> ram_banks=std::vector<std::uint8_t>(0x2000, 0);
    std::uint8_t *ram_bank = ram_banks.data();

    Cartridge(const std::string &filename)
    {
        load(filename);
    }

    void load(const std::string &filename);
    void setup_ram_bank();

    std::uint8_t read(std::uint16_t address);
    void write(std::uint16_t address, std::uint8_t value);
};

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

    std::uint8_t interrupt_triggered_register = 0; //0xFF0F
    std::uint8_t interrupt_enable_register = 0; //0xFF0F

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






    enum class InterruptFlag {
        VBLANK	= 1 << 0,
        LCDSTAT	= 1 << 1,
        TIMER	= 1 << 2,
        SERIAL	= 1 << 3,
        JOYPAD	= 1 << 4,
    };


    void trigger_interrupt(InterruptFlag interrupt);

    bool interrupts_master_enable_flag = true;

//    //FF0F
//    std::uint8_t interrupts_flags = 0;
//    //FFFF
//    std::uint8_t interrupts_enable = 0;


};

