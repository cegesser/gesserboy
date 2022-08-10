#include "bus.h"

#include <iostream>
#include <sstream>
#include <fstream>

const char *cartridge_type(const CartridgeHeader *header)
{
    static const char *types[0xFF+1] = { 0 };
    if (types[0x00] == nullptr)
    {
        types[0x00] = "ROM ONLY";
        types[0x01] = "MBC1";
        types[0x02] = "MBC1+RAM";
        types[0x03] = "MBC1+RAM+BATTERY";
        types[0x05] = "MBC2";
        types[0x06] = "MBC2+BATTERY";
        types[0x08] = "ROM+RAM 1";
        types[0x09] = "ROM+RAM+BATTERY 1";
        types[0x0B] = "MMM01";
        types[0x0C] = "MMM01+RAM";
        types[0x0D] = "MMM01+RAM+BATTERY";
        types[0x0F] = "MBC3+TIMER+BATTERY";
        types[0x10] = "MBC3+TIMER+RAM+BATTERY 2";
        types[0x11] = "MBC3";
        types[0x12] = "MBC3+RAM 2";
        types[0x13] = "MBC3+RAM+BATTERY 2";
        types[0x19] = "MBC5";
        types[0x1A] = "MBC5+RAM";
        types[0x1B] = "MBC5+RAM+BATTERY";
        types[0x1C] = "MBC5+RUMBLE";
        types[0x1D] = "MBC5+RUMBLE+RAM";
        types[0x1E] = "MBC5+RUMBLE+RAM+BATTERY";
        types[0x20] = "MBC6";
        types[0x22] = "MBC7+SENSOR+RUMBLE+RAM+BATTERY";
        types[0xFC] = "POCKET CAMERA";
        types[0xFD] = "BANDAI TAMA5";
        types[0xFE] = "HuC3";
        types[0xFF] = "HuC1+RAM+BATTERY";
    }
    return types[header->cartridge_type];
}

void print_header(const CartridgeHeader *header)
{
    std::cout << "Title    : " << header->title  << std::endl;
    std::cout << "Type     : " << int(header->cartridge_type) << ": " << cartridge_type(header) << std::endl;
    std::cout << "ROM Size : " << (32 << header->rom_size) << " KBytes"  << std::endl;
    std::cout << "RAM Size : " << int(header->ram_size) << std::endl;
}


void Cartridge::load(const std::string &filename)
{
    {
        std::ifstream fp(filename, std::ios::binary);

        if (!fp)
        {
            throw std::runtime_error("Unable to open file: " + filename);
        }

        fp.seekg(0, std::ios::end);
        std::uint64_t rom_size = fp.tellg();

        fp.seekg(0, std::ios::beg);
        rom_data.resize(rom_size);

        fp.read((char*)rom_data.data(), rom_data.size());

        rom_bank_ptr = rom_data.data()+0x4000;
    }

    header = new (&rom_data[0x100]) CartridgeHeader;

    header->title[15] = 0;

    std::uint16_t x = 0;
    for (std::uint16_t i=0x0134; i<=0x014C; i++)
    {
        x = x - rom_data[i] - 1;
    }

    if ( ! (x & 0xFF) )
    {
        throw std::runtime_error("Invalid header checksum");
    }
}

void Cartridge::setup_ram_bank()
{
    if (ram_banks.size() < 0x2000*(selected_ram_bank+1))
    {
        ram_banks.resize(0x2000*(selected_ram_bank+1));
    }

    ram_bank = &ram_banks[0x2000*selected_ram_bank];
}

uint8_t Cartridge::read(uint16_t address)
{
    //0000	3FFF	16 KiB ROM bank 00	From cartridge, usually a fixed bank
    if (0x0000 <= address && address  <= 0x3FFF)
    {
        return rom_data[address];
    }
    //4000	7FFF	16 KiB ROM Bank 01~NN	From cartridge, switchable bank via mapper (if any)
    if (0x4000 <= address && address  <= 0x7FFF)
    {
        return rom_bank_ptr[address-0x4000];
    }
    if (0xA000 <= address && address  <= 0xBFFF)
    {
        if (ram_enabled)
        {
            return ram_bank[address-0xA000];
        }

        return 0xFF;
    }

    std::ostringstream out;
    out <<  "Read from cart at " << std::hex << address << " not yet implemented";
    throw std::runtime_error(out.str());
}

void Cartridge::write(uint16_t address, uint8_t value)
{
    if (header->cartridge_type == 1 || header->cartridge_type == 2 || header->cartridge_type == 3)
    {
        //enable/disable ram
        if (0x0000 <= address && address  <= 0x1FFF)
        {
            ram_enabled = (value & 0xF) == 0xA;
            return;
        }
        //rom bank selection
        if (0x2000 <= address && address  <= 0x3FFF)
        {
            if (value == 0)
            {
                value = 1;
            }

            selected_rom_bank = value & 0b00011111;
            rom_bank_ptr = &rom_data[0x4000 * selected_rom_bank];
            return;
        }

        //RAM Bank Number - or - Upper Bits of ROM Bank Number (Write Only)
        if (0x4000 <= address && address  <= 0x5FFF)
        {
            //ram bank number
            selected_ram_bank = value & 0b11;

            if (ram_banking_mode)
            {
                setup_ram_bank();
            }
            return;
        }

        // Banking Mode Select (Write Only)
        if (0x6000 <= address && address  <= 0x7FFF)
        {
            ram_banking_mode = value & 1;

            if (ram_banking_mode)
            {
                setup_ram_bank();
            }
            return;
        }

    }
    else if (address < 0x7FFF)
    {
        return;
    }

    if (0xA000 <= address && address  <= 0xBFFF)
    {
        if ( ram_enabled)
        {
            ram_bank[address - 0xA000] = value;
        }

        return;
    }

    std::ostringstream out;
    out <<  "Write to cart at " << std::hex << address << " not yet implemented";
    throw std::runtime_error(out.str());
}

std::uint8_t serial_data = 0;
std::uint8_t serial_control = 0;
void serial_transfer_data_write(std::uint8_t data)
{
    serial_data = data;
    //std::cout << "DW " << std::hex << int(data) << std::endl;
}

std::uint8_t serial_transfer_data_read()
{
    //std::cout << "DR " << std::hex << int(serial_data) << std::endl;
    return serial_data;
}

void serial_transfer_control_write(std::uint8_t data)
{
    serial_control = data;
    //std::cout << "CW " <<  std::hex << int(data) << std::endl;
}

std::uint8_t serial_transfer_control_read()
{
    //std::cout << "CR " <<  std::hex << int(serial_control) << std::endl;
    return serial_control;
}

void Bus::trigger_interrupt(InterruptFlag interrupt)
{
    //const auto interrupts_enabled = read(0xFFFF);
    //if (interrupts_enabled & int(interrupt))
    {
        const auto interrupts_triggered = read(0xFF0F);
        write(0xFF0F, interrupts_triggered | int(interrupt));
    }
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

    if (address == 0xFF0F) return bus.interrupt_triggered_register;

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

    if (address == 0xFF0F) { bus.interrupt_triggered_register = value; return; }

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
        return interrupt_enable_register;
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
        std::cout << "int en "<< std::hex << (int)interrupt_enable_register << std::endl;
        interrupt_enable_register = value;
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

            this->trigger_interrupt(InterruptFlag::TIMER);
        }
    }
}
