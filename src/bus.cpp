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
    case 0xFF00: bus.p1_joypad = value; break;
    case 0xFF01: serial_transfer_data_write(value); break;
    case 0xFF02: serial_transfer_control_write(value); break;
    case 0xFF04: bus.timer.divider = 0; break;
    case 0xFF05: bus.timer.counter = value; break;
    case 0xFF06: bus.timer.modulo = value; break;
    case 0xFF07: bus.timer.control = value; break;
    case 0xFF0F: bus.interrupts_flags = value; break;
    case 0xFFFF: bus.interrupts_enable = value; break;

    default: if (address <= 0x7FFF)
        {
            if (1 <= bus.cart.header->cartridge_type && bus.cart.header->cartridge_type <= 3)
            {
                std::ostringstream out;
                out << "Writing to " << std::hex << address << " not yet implemented";
                //throw std::runtime_error(out.str());
            }
            //else ignore
        }
        else if (0x8000 <= address && address <= 0x9FFF)
        {
            bus.ppu.video_ram[address - 0x8000] = value;
        }
        else if (0xA000 <= address && address <= 0xBFFF)
        {
            bus.cartridge_ram[address - 0xA000] = value;
        }
        else if (0xC000 <= address && address <= 0xDFFF)
        {
            bus.work_ram[address - 0xC000] = value;
        }
        else if (0xE000 <= address && address <= 0xFDFF)
        {
            bus.work_ram[address - 0xE000] = value;
        }
        else if (0xFE00 <= address  && address <= 0xFEFF)
        {
            bus.obj_attribute_memory[address - 0xFE00] = value;
        }
        else if (0xFF40 <= address && address <= 0xFF4B)
        {
            bus.ppu.write(address, value);
        }
        else if (address == 0xFF7F)
        {
            std::cout << "Writing at " << std::hex << address << " " << (int)value << std::endl;;
            //throw std::logic_error("");
        }
        else if (0xFF10 <= address && address <= 0xFF26)
        {
            std::cout << "Writing to sound register " << std::hex <<  (address - 0xFF10) << std::endl;
        }
        else if (0xFF80 <= address && address <= 0xFFFE)
        {
            if (address == 0xFF80)
            {
                std::ostringstream out;
                out << "Writing to " << std::hex << address << ": " << (int)value ;
                std::cerr << out.str() << std::endl;
//                throw std::logic_error(out.str());
            }
            bus.high_ram[address - 0xFF80] = value;
        }
        else
        {
            if (address == 0xff71)
            {
                std::cout << "Writing from FF71 " << (int)value << std::endl;
                return *this;;
            }

            std::ostringstream out;
            out << "Writing to " << std::hex << address << " not yet implemented";
            //throw std::runtime_error(out.str());
        }
    }

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
    case 0xFF0F: return bus.interrupts_flags;
    case 0xFFFF: return bus.interrupts_enable;
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

    if (0xFE00 <= address  && address <= 0xFEFF)
    {
        return bus.obj_attribute_memory[address - 0xFE00];
    }

    if (0xFF40 <= address && address <= 0xFF4B)
    {
        return bus.ppu.read(address);
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



bool Bus::int_trigger(IntType interrupt)
{
    if (interrupts_enable & interrupt)
    {
        interrupts_flags |= interrupts_flags;
        return true;
    }
    return false;
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

            this->int_trigger(int_TIMER);
        }
    }
}
