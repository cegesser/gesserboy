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
    //std::cout << "Title    : " << header->title  << std::endl;
    //std::cout << "Type     : " << int(header->cartridge_type) << ": " << cartridge_type(header) << std::endl;
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
        data.resize(rom_size);

        fp.read((char*)data.data(), data.size());
    }

    header = new (&data[0x100]) CartridgeHeader;

    header->title[15] = 0;

    std::uint16_t x = 0;
    for (std::uint16_t i=0x0134; i<=0x014C; i++)
    {
        x = x - data[i] - 1;
    }

    if ( ! (x & 0xFF) )
    {
        throw std::runtime_error("Invalid header checksum");
    }
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


Bus::Ref &Bus::Ref::operator=(uint8_t value)
{
    if (address == 0xFF0F || address == 0xFFFF)
        std::cout << "Writing to " << std::hex << address << ": " << (int)value << std::endl;
    switch (address)
    {
    case 0xFF00: bus.p1_joypad = value; return *this;
    case 0xFF01: serial_transfer_data_write(value); return *this;
    case 0xFF02: serial_transfer_control_write(value); return *this;
    case 0xFF04: bus.timer.divider = 0; return *this;
    case 0xFF05: bus.timer.counter = value; return *this;
    case 0xFF06: bus.timer.modulo = value; return *this;
    case 0xFF07: bus.timer.control = value; return *this;
    case 0xFF0F: bus.interrupt_triggered_register = value; return *this;
    case 0xFFFF: bus.interrupt_enable_register = value; return *this;
    }

    if (address <= 0x7FFF)
    {
        if (1 <= bus.cart.header->cartridge_type && bus.cart.header->cartridge_type <= 3)
        {
            std::ostringstream out;
            out << "Writing to " << std::hex << address << " not yet implemented";
            //throw std::runtime_error(out.str());
        }
        return *this;
        //else ignore
    }

    if (0x8000 <= address && address <= 0x9FFF)
    {
        bus.ppu.video_ram[address - 0x8000] = value;
        return *this;
    }

    if (0xA000 <= address && address <= 0xBFFF)
    {
        bus.cartridge_ram[address - 0xA000] = value;
        return *this;
    }

    if (0xC000 <= address && address <= 0xDFFF)
    {
        bus.work_ram[address - 0xC000] = value;
        return *this;
    }

    if (0xE000 <= address && address <= 0xFDFF)
    {
        bus.work_ram[address - 0xE000] = value;
        return *this;
    }

    if (0xFE00 <= address  && address <= 0xFEFF)
    {
        bus.ppu.obj_attribute_memory[address - 0xFE00] = value;
        return *this;
    }

    if (0xFF40 <= address && address <= 0xFF4B)
    {
        //bus.ppu.write(address, value);
        return *this;
    }

    if (address == 0xFF7F)
    {
        std::cout << "Writing at " << std::hex << address << " " << (int)value << std::endl;;
        //throw std::logic_error("");
        return *this;
    }

    if (0xFF10 <= address && address <= 0xFF26)
    {
        std::cout << "Writing to sound register " << std::hex <<  (address - 0xFF10) << std::endl;
        return *this;
    }

    if (0xFF80 <= address && address <= 0xFFFE)
    {
        if (address == 0xFF80)
        {
            std::ostringstream out;
            out << "Writing to " << std::hex << address << ": " << (int)value ;
            std::cerr << out.str() << std::endl;
            //                throw std::logic_error(out.str());
        }
        bus.high_ram[address - 0xFF80] = value;
        return *this;
    }

    if (address == 0xff71)
    {
        std::cout << "Writing from FF71 " << (int)value << std::endl;
        return *this;;
    }

    std::ostringstream out;
    out << "Writing to " << std::hex << address << " not yet implemented";
    //throw std::runtime_error(out.str());

    return *this;
}

std::uint8_t read_from_address(const Bus &bus, std::uint16_t address)
{
    //if (address == 0xFF0F || address == 0xFFFF)
    //    std::cout << "Reading from " << std::hex << address << ": " << int(address == 0xFF0F
    //                                                                     ? bus.interrupts_flags
    //                                                                     : bus.interrupts_enable )<< std::endl;
    switch (address)
    {
    case 0xFF00: return bus.p1_joypad;
    case 0xFF01: return serial_transfer_data_read(); break;
    case 0xFF02: return serial_transfer_control_read(); break;
    case 0xFF04: return bus.timer.divider;
    case 0xFF05: return bus.timer.counter;
    case 0xFF06: return bus.timer.modulo;
    case 0xFF07: return bus.timer.control;
    case 0xFF0F: return bus.interrupt_triggered_register;
    case 0xFFFF: return bus.interrupt_enable_register;
    }

    if (address <= 0x7FFF)
    {
        return bus.cart.data[address];
    }

    if (0x8000 <= address && address <= 0x9FFF)
    {
        return bus.ppu.video_ram[address - 0x8000];
    }

    if (0xA000 <= address && address <= 0xBFFF)
    {
        return bus.cartridge_ram[address - 0xA000];
    }

    if (0xC000 <= address && address <= 0xDFFF)
    {
        return bus.work_ram[address - 0xC000];
    }

    if (0xE000 <= address && address <= 0xFDFF)
    {
        return bus.work_ram[address - 0xE000];
    }

    if (0xFE00 <= address  && address <= 0xFE9F)
    {
        return bus.ppu.obj_attribute_memory[address - 0xFE00];
    }

    if (0xFF40 <= address && address <= 0xFF4B)
    {
        //return bus.ppu.read(address);
    }

    if (0xFF80 <= address && address <= 0xFFFE)
    {
        return bus.high_ram[address - 0xFF80];
    }

    if (address == 0xff71)
    {
        std::cout << "reading from FF71" << std::endl;
        return 0;
    }

    std::ostringstream out;
    out <<  "Reading from " << std::hex << address << " not yet implemented";
    return 0;
    //throw std::runtime_error(out.str());
}


Bus::Ref::operator std::uint8_t() const
{
    auto value = read_from_address(bus, address);

    if (address > 0x7FFF)
    {
        //std::cout << "Reading from " << std::hex << address << ": " << (int)value << "\n";
    }

    return value;
}



void Bus::trigger_interrupt(InterruptFlag interrupt)
{
    const auto interrupts_enabled = read(0xFFFF);
    if (interrupts_enabled & int(interrupt))
    {
        const auto interrupts_triggered = read(0xFF0F);
        write(0xFF0F, interrupts_triggered | int(interrupt));
    }
}



uint8_t Bus::read(std::uint16_t address)
{
    if (address == 0xff04)
    {
        //_CrtDbgBreak();
    }

    //return Ref{*this, address};
    //0000	3FFF	16 KiB ROM bank 00	From cartridge, usually a fixed bank
    if (address <= 0x3FFF)
    {
        return cart.data[address];
    }
    //4000	7FFF	16 KiB ROM Bank 01~NN	From cartridge, switchable bank via mapper (if any)
    if (0x4000 <= address && address <= 0x7FFF)
    {
        return cart.data[address];
    }
    //8000	9FFF	8 KiB Video RAM (VRAM)	In CGB mode, switchable bank 0/1
    if (0x8000 <= address && address <= 0x9FFF)
    {
        return ppu.video_ram[address - 0x8000];
    }
    //A000	BFFF	8 KiB External RAM	From cartridge, switchable bank if any
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
        return read(address - 0x2000);
    }
    //FE00	FE9F	Sprite attribute table (OAM)
    //FEA0	FEFF	Not Usable	Nintendo says use of this area is prohibited
    //FF00	FF7F	I/O Registers
    if (0xFF00 <= address && address <= 0xFF7F)
    {
        //$FF00		DMG	Joypad input
        if (address == 0xFF00)
            return /*p1_joypad */ 0xCF;
        //$FF01	$FF02	DMG	Serial transfer
        if (address == 0xFF01) return serial_transfer_data_read();
        if (address == 0xFF02) return serial_transfer_control_read();

        //$FF04	$FF07	DMG	Timer and divider
        //$FF10	$FF26	DMG	Sound
        //$FF30	$FF3F	DMG	Wave pattern
        //$FF40	$FF4B	DMG	LCD Control, Status, Position, Scrolling, and Palettes
        if (address == 0xFF40) return ppu.read(address);
        if (address == 0xFF44) return ppu.read(address);
        if (address == 0xFF4D) return 0xFF; //FF4D - KEY1 - CGB Mode Only - Prepare Speed Switch
        //$FF4F		CGB	VRAM Bank Select
        //$FF50		DMG	Set to non-zero to disable boot ROM
        //$FF51	$FF55	CGB	VRAM DMA
        //$FF68	$FF69	CGB	BG / OBJ Palettes
        //$FF70		CGB	WRAM Bank Select

        if (address == 0xFF0F) return interrupt_triggered_register;

        //return io_range[address-0xFF00];
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
    throw std::runtime_error(out.str());
    return 0;
}

void Bus::write(uint16_t address, uint8_t value)
{
    //Ref{*this, address} = value; return;

    if (address == 0xff04)
    {
       // _CrtDbgBreak();
    }

    //0000	3FFF	16 KiB ROM bank 00	From cartridge, usually a fixed bank
    if (0x0000 <= address && address  <= 0x3FFF)
    {
        cart.data[address] = value;
        return;
    }
    //4000	7FFF	16 KiB ROM Bank 01~NN	From cartridge, switchable bank via mapper (if any)
    if (0x4000 <= address && address  <= 0x7FFF)
    {
        cart.data[address] = value;
        return;
    }
    //8000	9FFF	8 KiB Video RAM (VRAM)	In CGB mode, switchable bank 0/1
    if (0x8000 <= address && address <= 0x9FFF)
    {
        ppu.video_ram[address - 0x8000] = value;
        return;
    }
    //A000	BFFF	8 KiB External RAM	From cartridge, switchable bank if any
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
        //std::cout << "Writing to forbidden zone at " << std::hex << address << ": " << (int)value << "\n";
        return;
    }
    //FF00	FF7F	I/O Registers
    if (0xFF00 <= address && address <= 0xFF7F)
    {
        //$FF00		DMG	Joypad input
        if (address == 0xFF00) {
            p1_joypad = value; return; }
        //$FF01	$FF02	DMG	Serial transfer
        if (address == 0xFF01) { serial_transfer_data_write(value); return; }
        if (address == 0xFF02) { serial_transfer_control_write(value); return; }

        //$FF04	$FF07	DMG	Timer and divider
        if (address == 0xFF05) { timer.counter = value; return; }
        if (address == 0xFF06) { timer.modulo  = value; return; }
        if (address == 0xFF07) { timer.control = value; return; }

        if (address == 0xFF0F) { interrupt_triggered_register = value; return; }

        //$FF10	$FF26	DMG	Sound
        if (0xFF10 <= address && address <= 0xFF26) { return; }
        //$FF30	$FF3F	DMG	Wave pattern
        if (0xFF30 <= address && address <= 0xFF3F) { return; }
        //$FF40	$FF4B	DMG	LCD Control, Status, Position, Scrolling, and Palettes

        if (address == 0xFF40) { ppu.write(address, value); return; }
        if (address == 0xFF41) { ppu.write(address, value); return; }
        if (address == 0xFF42) { ppu.write(address, value); return; }
        if (address == 0xFF43) { ppu.write(address, value); return; }
        if (address == 0xFF46) { ppu.write(address, value); return; }
        if (address == 0xFF47) { ppu.write(address, value); return; }
        if (address == 0xFF48) { ppu.write(address, value); return; }
        if (address == 0xFF49) { ppu.write(address, value); return; }
        if (address == 0xFF4A) { ppu.write(address, value); return; }
        if (address == 0xFF4B) { ppu.write(address, value); return; }

        //$FF4F		CGB	VRAM Bank Select
        //$FF50		DMG	Set to non-zero to disable boot ROM
        //$FF51	$FF55	CGB	VRAM DMA
        //$FF68	$FF69	CGB	BG / OBJ Palettes
        //$FF70		CGB	WRAM Bank Select



        if (address == 0xFF4d) return;
        if (address == 0xFF7F) { return; }//Mistery
        //io_range[address-0xFF00] = value;
        //return;
    }
    //FF80	FFFE	High RAM (HRAM)
    if (0xFF80 <= address && address <= 0xFFFE)
    {
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
        case 0b00:
            timer_update = (prev_div & (1 << 9)) && (!(timer.divider & (1 << 9)));
            break;
        case 0b01:
            timer_update = (prev_div & (1 << 3)) && (!(timer.divider & (1 << 3)));
            break;
        case 0b10:
            timer_update = (prev_div & (1 << 5)) && (!(timer.divider & (1 << 5)));
            break;
        case 0b11:
            timer_update = (prev_div & (1 << 7)) && (!(timer.divider & (1 << 7)));
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
