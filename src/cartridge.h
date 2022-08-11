#pragma once

#include <vector>
#include <string>
#include <memory>

struct CartridgeHeader {
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

struct MemoryBankController
{
    uint8_t *rom;
    uint8_t *ram;

    virtual uint8_t read(uint16_t address) = 0;
    virtual void write(uint16_t address, uint8_t value) = 0;
    virtual ~MemoryBankController() = default;
};

struct Cartridge
{
    std::unique_ptr<MemoryBankController> mbc;
    std::vector<std::uint8_t> rom_data;
    std::vector<std::uint8_t> ram_banks;
    CartridgeHeader *header;

    Cartridge(const std::string &filename)
    {
        load(filename);
    }

    void load(const std::string &filename);

    std::uint8_t read(std::uint16_t address)
    {
        return mbc->read(address);
    }

    void write(std::uint16_t address, std::uint8_t value)
    {
        return mbc->write(address, value);
    }
};
