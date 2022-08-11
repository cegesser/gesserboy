#include "bus.h"

#include <iostream>
#include <sstream>
#include <fstream>


std::uint8_t serial_data = 0;
std::uint8_t serial_control = 0;
void serial_transfer_data_write(std::uint8_t data)
{
    serial_data = data;
}

std::uint8_t serial_transfer_data_read()
{
    return serial_data;
}

void serial_transfer_control_write(std::uint8_t data)
{
    serial_control = data;
}

std::uint8_t serial_transfer_control_read()
{
    return serial_control;
}

uint8_t io_read(Bus &bus, std::uint16_t address)
{
    //$FF00		DMG	Joypad input
    if (address == 0xFF00)
    {
        uint8_t output = 0xCF;

        bool dir_sel = bus.p1_joypad.query & 0x10;
        bool btn_sel = bus.p1_joypad.query & 0x20;

        if (!btn_sel) {
            if (bus.p1_joypad.start) {
                output &= ~(1 << 3);
            }
            if (bus.p1_joypad.select) {
                output &= ~(1 << 2);
            }
            if (bus.p1_joypad.a) {
                output &= ~(1 << 0);
            }
            if (bus.p1_joypad.b) {
                output &= ~(1 << 1);
            }
        }

        if (!dir_sel) {
            if (bus.p1_joypad.left) {
                output &= ~(1 << 1);
            }
            if (bus.p1_joypad.right) {
                output &= ~(1 << 0);
            }
            if (bus.p1_joypad.up) {
                output &= ~(1 << 2);
            }
            if (bus.p1_joypad.down) {
                output &= ~(1 << 3);
            }
        }

        return output;
    }


    //$FF01	$FF02	DMG	Serial transfer
    if (address == 0xFF01) return serial_transfer_data_read();
    if (address == 0xFF02) return serial_transfer_control_read();

    //$FF04	$FF07	DMG	Timer and divider
    if (address == 0xFF04) return bus.timer.divider;
    if (address == 0xFF05) return bus.timer.counter;
    if (address == 0xFF06) return bus.timer.modulo;
    if (address == 0xFF07) return bus.timer.control;
    //$FF10	$FF26	DMG	Sound
    //$FF30	$FF3F	DMG	Wave pattern
    //$FF40	$FF4B	DMG	LCD Control, Status, Position, Scrolling, and Palettes
    if (address == 0xFF40) return bus.ppu.read(address);
    if (address == 0xFF41) return bus.ppu.read(address);
    if (address == 0xFF44) return bus.ppu.read(address);
    if (address == 0xFF4D) return 0xFF; //FF4D - KEY1 - CGB Mode Only - Prepare Speed Switch
    //$FF4F		CGB	VRAM Bank Select
    //$FF50		DMG	Set to non-zero to disable boot ROM
    //$FF51	$FF55	CGB	VRAM DMA
    //$FF68	$FF69	CGB	BG / OBJ Palettes
    //$FF70		CGB	WRAM Bank Select

    if (address == 0xFF0F) return bus.interrupts.trigger_register;

    std::ostringstream out;
    out <<  "Reading IO from " << std::hex << address << " not yet implemented";
    throw std::runtime_error(out.str());
}

void io_write(Bus &bus, std::uint16_t address, uint8_t value)
{
    //$FF00		DMG	Joypad input
    if (address == 0xFF00) {
        bus.p1_joypad.query = value; return; }

    //$FF01	$FF02	DMG	Serial transfer
    if (address == 0xFF01) { serial_transfer_data_write(value); return; }
    if (address == 0xFF02) { serial_transfer_control_write(value); return; }

    //$FF04	$FF07	DMG	Timer and divider
    if (address == 0xFF04) { bus.timer.divider = 0; return; };
    if (address == 0xFF05) { bus.timer.counter = value; return; }
    if (address == 0xFF06) { bus.timer.modulo  = value; return; }
    if (address == 0xFF07) { bus.timer.control = value; return; }

    if (address == 0xFF0F) { bus.interrupts.trigger_register = value; return; }

    //$FF10	$FF26	DMG	Sound
    if (0xFF10 <= address && address <= 0xFF26) { return; }
    //$FF30	$FF3F	DMG	Wave pattern
    if (0xFF30 <= address && address <= 0xFF3F) { return; }
    //$FF40	$FF4B	DMG	LCD Control, Status, Position, Scrolling, and Palettes

    if (address == 0xFF40) { bus.ppu.write(address, value); return; }
    if (address == 0xFF41) { bus.ppu.write(address, value); return; }
    if (address == 0xFF42) { bus.ppu.write(address, value); return; }
    if (address == 0xFF43) { bus.ppu.write(address, value); return; }
    if (address == 0xFF46) { bus.ppu.write(address, value); return; }
    if (address == 0xFF47) { bus.ppu.write(address, value); return; }
    if (address == 0xFF48) { bus.ppu.write(address, value); return; }
    if (address == 0xFF49) { bus.ppu.write(address, value); return; }
    if (address == 0xFF4A) { bus.ppu.write(address, value); return; }
    if (address == 0xFF4B) { bus.ppu.write(address, value); return; }

    //$FF4F		CGB	VRAM Bank Select
    //$FF50		DMG	Set to non-zero to disable boot ROM
    //$FF51	$FF55	CGB	VRAM DMA
    //$FF68	$FF69	CGB	BG / OBJ Palettes
    //$FF70		CGB	WRAM Bank Select


    if (address == 0xFF03 || address == 0xFF4d || address == 0xFF7F) {
        std::cout << std::hex << "Writing to " << address << " value " << int(value) << "\n";
        return;
    }
}

uint8_t Bus::read(std::uint16_t address)
{
    //0000	3FFF	16 KiB ROM bank 00	From cartridge, usually a fixed bank
    if (0x0000 <= address && address  <= 0x3FFF)
    {
        return cart.read(address);
    }
    //4000	7FFF	16 KiB ROM Bank 01~NN	From cartridge, switchable bank via mapper (if any)
    if (0x4000 <= address && address <= 0x7FFF)
    {
        return cart.read(address);
    }
    //8000	9FFF	8 KiB Video RAM (VRAM)	In CGB mode, switchable bank 0/1
    if (0x8000 <= address && address <= 0x9FFF)
    {
        return ppu.video_ram[address - 0x8000];
    }
    //A000 BFFF	8 KiB External RAM	From cartridge, switchable bank if any
    if (0xA000 <= address && address <= 0xBFFF)
    {
        return cart.read(address);
    }
    //C000	CFFF	4 KiB Work RAM (WRAM)
    if (0xC000 <= address && address <= 0xCFFF)
    {
        return work_ram1[address-0xC000];
    }
    //D000	DFFF	4 KiB Work RAM (WRAM)	In CGB mode, switchable bank 1~7
    if (0xD000 <= address && address <= 0xDFFF)
    {
        return work_ram2[address-0xD000];
    }
    //E000	FDFF	Mirror of C000~DDFF (ECHO RAM)	Nintendo says use of this area is prohibited.
    if (0xE000 <= address && address <= 0xFDFF)
    {
        //return read(address - 0x2000);
    }
    //FE00	FE9F	Sprite attribute table (OAM)
    //FEA0	FEFF	Not Usable	Nintendo says use of this area is prohibited
    //FF00	FF7F	I/O Registers
    if (0xFF00 <= address && address <= 0xFF7F)
    {
        return io_read(*this, address);
    }
    //FF80	FFFE	High RAM (HRAM)
    if (0xFF80 <= address && address <= 0xFFFE)
    {
        return high_ram[address-0xFF80];
    }
    //FFFF	FFFF	Interrupt Enable register (IE)
    if (0xFFFF <= address && address <= 0xFFFF)
    {
        return interrupts.enable_register;
    }

    std::ostringstream out;
    out <<  "Reading from " << std::hex << address << " not yet implemented";
    std::cerr << out.str();
    exit(1);
    //throw std::runtime_error(out.str());
}

void Bus::write(uint16_t address, uint8_t value)
{
    //0000	3FFF	16 KiB ROM bank 00	From cartridge, usually a fixed bank
    if (0x0000 <= address && address  <= 0x3FFF)
    {
        return cart.write(address, value);
    }
    //4000	7FFF	16 KiB ROM Bank 01~NN	From cartridge, switchable bank via mapper (if any)
    if (0x4000 <= address && address  <= 0x7FFF)
    {
        return cart.write(address, value);
    }
    //8000	9FFF	8 KiB Video RAM (VRAM)	In CGB mode, switchable bank 0/1
    if (0x8000 <= address && address <= 0x9FFF)
    {
        ppu.video_ram[address - 0x8000] = value;
        return;
    }
    //A000	BFFF	8 KiB External RAM	From cartridge, switchable bank if any
    if (0xA000 <= address && address <= 0xBFFF)
    {
        return cart.write(address, value);
    }
    //C000	CFFF	4 KiB Work RAM (WRAM)
    if (0xC000 <= address && address <= 0xCFFF)
    {
        work_ram1[address-0xC000] = value;
        return;
    }
    //D000	DFFF	4 KiB Work RAM (WRAM)	In CGB mode, switchable bank 1~7
    if (0xD000 <= address && address <= 0xDFFF)
    {
        work_ram2[address-0xD000] = value;
        return;
    }
    //E000	FDFF	Mirror of C000~DDFF (ECHO RAM)	Nintendo says use of this area is prohibited.
    if (0xE000 <= address && address <= 0xFDFF)
    {
        write(address - 0x2000, value);
        return;
    }
    //FE00	FE9F	Sprite attribute table (OAM)
    if (0xFE00 <= address  && address <= 0xFE9F)
    {
        ppu.obj_attribute_memory[address - 0xFE00] = value;
        return;
    }
    //FEA0	FEFF	Not Usable	Nintendo says use of this area is prohibited
    if (0xFEA0 <= address  && address <= 0xFEFF)
    {
        std::cout << "Writing to forbidden zone at " << std::hex << address << ": " << (int)value << "\n";
        return;
    }
    //FF00	FF7F	I/O Registers
    if (0xFF00 <= address && address <= 0xFF7F)
    {
        io_write(*this, address, value);
        return;
    }
    //FF80	FFFE	High RAM (HRAM)
    if (0xFF80 <= address && address <= 0xFFFE)
    {
        if (0xFFb6 <= address && address <= (0xFFb6+0x0c) && value == 0x2f)
        {

            //std::cout << "writing to " << std::hex << address << " <- " << int(value) << "\n";

        }

        high_ram[address-0xFF80] = value;
        //if (address == 0xffe3) throw std::logic_error("");
        return;
    }
    //FFFF	FFFF	Interrupt Enable register (IE)
    if (address == 0xFFFF)
    {
        interrupts.enable_register = value;
        return;
    }

    std::ostringstream out;
    out <<  "Write to " << std::hex << address << " not yet implemented";
    throw std::runtime_error(out.str());
}

void Bus::run_timer_once()
{
    auto prev_div = timer.divider;

    ++timer.divider;

    auto timer_update = false;

    switch (timer.control & (0b11)) {
        case 0b00: // Clock / 1024
            timer_update = (prev_div & (1 << 9)) && (!(timer.divider & (1 << 9)));
            break;
        case 0b11: // Clock / 256
            timer_update = (prev_div & (1 << 7)) && (!(timer.divider & (1 << 7)));
            break;
        case 0b10:  // Clock / 64
            timer_update = (prev_div & (1 << 5)) && (!(timer.divider & (1 << 5)));
            break;
        case 0b01: // Clock / 16
            timer_update = (prev_div & (1 << 3)) && (!(timer.divider & (1 << 3)));
            break;
    }

    if (timer_update && timer.control & (1 << 2)) {
        ++timer.counter;

        if (timer.counter == 0xFF) {
            timer.counter = timer.modulo;

            interrupts.trigger_interrupt(Interrupts::TIMER);
        }
    }
}
