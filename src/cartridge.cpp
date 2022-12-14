#include "cartridge.h"
#include <iostream>
#include <fstream>
#include <sstream>

struct MBC0 : MemoryBankController
{
    uint8_t read(uint16_t address) override
    {
        if (address <= 0x7FFF)
        {
            return rom[address];
        }
        return 0;
    }

    void write(uint16_t address, uint8_t value) override
    {

    }
};

struct MBC1 : MemoryBankController
{
    bool ram_enabled = false;
    bool rom_banking_mode = false;
    uint8_t selected_rom_bank = 1;
    uint8_t selected_ram_bank = 0;

    uint8_t read(uint16_t address) override
    {
        if (0x0000 <= address && address <= 0x3FFF)
        {
            if (rom_banking_mode)
            {
                auto rom_bank_0 = selected_rom_bank & 0b11100000;
                auto bank_address = address + rom_bank_0 * 0x4000;
                return rom[bank_address & (rom_size - 1)];
            }
            else
            {
                return rom[address];
            }
        }

        if (0x4000 <= address && address <= 0x7FFF)
        {
            auto bank_address = (address - 0x4000) + selected_rom_bank * 0x4000;
            return rom[bank_address & (rom_size - 1)];
        }

        if (0xA000 <= address && address <= 0xBFFF)
        {
            if ( ram_enabled )
            {
                auto ram_address = rom_banking_mode
                        ? address - 0xA000 + selected_ram_bank * 0x2000
                        : address - 0xA000;

                return ram[ram_address];
            }
            return 0xFF;
        }

        std::ostringstream out;
        out <<  "Read from cart at " << std::hex << address << " not yet implemented";
        throw std::runtime_error(out.str());
    }

    void write(uint16_t address, uint8_t value) override
    {
        //0000-1FFF - RAM Enable (Write Only)
        if (0x0000 <= address && address  <= 0x1FFF)
        {
            ram_enabled = (value & 0xF) == 0xA;
            return;
        }

        //2000-3FFF - ROM Bank Number (Write Only)
        if (0x2000 <= address && address  <= 0x3FFF)
        {
            selected_rom_bank &= 0b11100000;
            selected_rom_bank |= value & 0b00011111 ;

            if ((selected_rom_bank & 0x1F) == 0)
            {
                ++selected_rom_bank;
            }
            //std::cout << "Selected ROM bank " << std::dec << (int)selected_rom_bank << "="<<(selected_rom_bank*0x4000)<< "\n";
            return;
        }

        //4000-5FFF - RAM Bank Number - or - Upper Bits of ROM Bank Number (Write Only)
        if (0x4000 <= address && address  <= 0x5FFF)
        {
            selected_ram_bank = value & 0b11 ;
            if (selected_ram_bank * 0x2000 >= ram_size)
            {
                auto max_bank = ram_size/0x2000;
                selected_ram_bank &= (max_bank - 1);
            }

            selected_rom_bank &= 0b00011111;
            selected_rom_bank |= value << 5;

            return;
        }

        //6000-7FFF - Banking Mode Select (Write Only)
        if (0x6000 <= address && address  <= 0x7FFF)
        {
            uint8_t mode = value & 0x1 ;
            rom_banking_mode = (mode != 0);
            return;
        }

        if (0xA000 <= address && address <= 0xBFFF)
        {
            if (ram_enabled)
            {
                auto ram_address = rom_banking_mode
                        ? address - 0xA000 + selected_ram_bank * 0x2000
                        : address - 0xA000;

                ram[ram_address] = value;
                battery_dirty = true;
            }
            return;
        }

        std::ostringstream out;
        out <<  "Write to cart at " << std::hex << address << " not yet implemented";
        throw std::runtime_error(out.str());
    }
};

struct MBC3 : MBC1
{
    uint8_t read(uint16_t address) override
    {
        if (0x0000 <= address && address <= 0x3FFF)
        {
            return rom[address];
        }

        if (0x4000 <= address && address <= 0x7FFF)
        {
            return rom[selected_rom_bank * 0x4000 + address - 0x4000];
        }

        if(0xA000 <= address && address <= 0xBFFF)
        {
            if (ram_enabled && selected_ram_bank <= 3)
            {
                return ram[selected_ram_bank * 0x2000 + address - 0xA000];
            }
        }

        return 0;
    }

    void write(uint16_t address, uint8_t value) override
    {
        //0000-1FFF - RAM and Timer Enable (Write Only)
        if (0x0000 <= address && address  <= 0x1FFF)
        {
            ram_enabled = (value & 0x0F) == 0x0A;
            return;
        }

        //2000-3FFF - ROM Bank Number (Write Only)
        if (0x2000 <= address && address  <= 0x3FFF)
        {
            selected_rom_bank = value & 0x7F;
            if (selected_rom_bank == 0)
            {
                selected_rom_bank = 1;
            }
            return;
        }

        //4000-5FFF - RAM Bank Number - or - RTC Register Select (Write Only)
        if (0x4000 <= address && address  <= 0x5FFF)
        {
            selected_ram_bank = value & 0xF;
            return;
        }

        if (0xA000 <= address && address <= 0xBFFF)
        {
            if (ram_enabled && selected_ram_bank <= 3)
            {
                ram[selected_ram_bank * 0x2000 + address - 0xA000] = value;
                battery_dirty = true;
                return;
            }
        }
    }
};

struct MBC5 : MBC1
{
    uint8_t read(uint16_t address) override
    {
        if (0x0000 <= address && address <= 0x3FFF)
        {
            return rom[address];
        }

        if (0x4000 <= address && address <= 0x7FFF)
        {
            return rom[selected_rom_bank * 0x4000 + address - 0x4000];
        }

        if(0xA000 <= address && address <= 0xBFFF)
        {
            if (ram_enabled)
            {
                return ram[selected_ram_bank * 0x2000 + address - 0xA000];
            }
        }

        return 0;
    }

    void write(uint16_t address, uint8_t value) override
    {
        //0000-1FFF - RAM Enable (Write Only)
        if (0x0000 <= address && address  <= 0x1FFF)
        {
            ram_enabled = (value & 0x0F) == 0x0A;
            return;
        }

        //2000-2FFF - 8 least significant bits of ROM bank number (Write Only)
        if (0x2000 <= address && address  <= 0x2FFF)
        {
            selected_rom_bank = (selected_rom_bank & 0x100) | value;
            return;
        }

        //3000-3FFF - 9th bit of ROM bank number (Write Only)
        if (0x3000 <= address && address  <= 0x3FFF)
        {
            selected_rom_bank = (selected_rom_bank & 0xFF) | ((value & 1) << 8);
            return;
        }

        //4000-5FFF - RAM bank number (Write Only)
        if (0x4000 <= address && address  <= 0x5FFF)
        {
            selected_ram_bank = value & 0xF;
            return;
        }

        if (0xA000 <= address && address <= 0xBFFF)
        {
            if (ram_enabled)
            {
                ram[selected_ram_bank * 0x2000 + address - 0xA000] = value;
                battery_dirty = true;
                return;
            }
        }
    }
};

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


bool has_battery(const CartridgeHeader *header)
{
    switch (header->cartridge_type)
    {
        case 0x03: return true; //MBC1+RAM+BATTERY
        case 0x06: return true; //MBC2+BATTERY
        case 0x09: return true; //ROM+RAM+BATTERY 1
        case 0x0D: return true; //MMM01+RAM+BATTERY
        case 0x0F: return true; //MBC3+TIMER+BATTERY
        case 0x10: return true; //MBC3+TIMER+RAM+BATTERY 2
        case 0x13: return true; //MBC3+RAM+BATTERY 2
        case 0x1B: return true; //MBC5+RAM+BATTERY
        case 0x1E: return true; //MBC5+RUMBLE+RAM+BATTERY
        case 0x22: return true; //MBC7+SENSOR+RUMBLE+RAM+BATTERY
        case 0xFF: return true; //HuC1+RAM+BATTERY
    }

    return false;
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

    switch (header->ram_size)
    {
        case 2: ram_banks.resize(0x2000 *  1); break;
        case 3: ram_banks.resize(0x2000 *  4); break;
        case 4: ram_banks.resize(0x2000 * 16); break;
        case 5: ram_banks.resize(0x2000 *  8); break;
    }

    mbc = [&]() -> std::unique_ptr<MemoryBankController> {
        switch (header->cartridge_type) {
            case 0x00: return std::make_unique<MBC0>();

            case 0x01: return std::make_unique<MBC1>();
            case 0x02: return std::make_unique<MBC1>();
            case 0x03: return std::make_unique<MBC1>(); //BATTERY

            case 0x0F: return std::make_unique<MBC3>();
            case 0x10: return std::make_unique<MBC3>();
            case 0x11: return std::make_unique<MBC3>();
            case 0x12: return std::make_unique<MBC3>();
            case 0x13: return std::make_unique<MBC3>(); //BATTERY 2

            case 0x19: return std::make_unique<MBC5>();
            case 0x1A: return std::make_unique<MBC5>();
            case 0x1B: return std::make_unique<MBC5>(); //BATTERY
            case 0x1C: return std::make_unique<MBC5>();
            case 0x1D: return std::make_unique<MBC5>();
            case 0x1E: return std::make_unique<MBC5>(); //BATTERY

            default: throw std::runtime_error("Cartridge type not supported: "+std::to_string(header->cartridge_type)+" "+cartridge_type(header));
        }
    }();

    mbc->rom = rom_data.data();
    mbc->rom_size = rom_data.size();
    mbc->ram = ram_banks.data();
    mbc->ram_size = ram_banks.size();
}

void Cartridge::save_battery()
{
    if ( ! has_battery(header) )
    {
        return;
    }

    std::ofstream out(std::string(header->title)+".battery", std::ios::binary);
    out.write(reinterpret_cast<const char*>(ram_banks.data()), ram_banks.size());
    mbc->battery_dirty = false;
}

void Cartridge::load_battery()
{
    if ( ! has_battery(header) )
    {
        return;
    }

    std::ifstream in(std::string(header->title)+".battery", std::ios::binary);
    in.read(reinterpret_cast<char*>(ram_banks.data()), ram_banks.size());
}

