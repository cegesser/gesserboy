#include "system.h"
#include <iostream>
#include <sstream>

System::System(const std::string &cartridge_filename)
    : timer{ interrupts }
    , cart(cartridge_filename)
    , bus{ interrupts, timer, ppu, cart }
    , cpu{ bus }
    , ppu{ bus }
{
    std::cout << "Title    : " << cart.header->title  << std::endl;
    std::cout << "Type     : " << int(cart.header->cartridge_type) << ": " << cartridge_type(cart.header) << std::endl;
    std::cout << "ROM Size : " << (32 << cart.header->rom_size) << " KBytes"  << std::endl;
    std::cout << "RAM Size : " << int(cart.header->ram_size) << std::endl;
    std::cout << "Cart Size : " << (cart.rom_data.size()) << " Bytes"  << std::endl;
}

void System::tick()
{
    size_t ticks = 0;
    ticks += cpu.run_interrupts();
    ticks += cpu.run_once();
    for (size_t i=0; i<ticks; ++i)
    {
        bus.timer.run_once();
        ppu.run_ounce();
    }
//    log.push_back(cpu.state_str() + " | " + cpu.last_inst_str);
//    if (log.size() > 100000)
//    {
//        log.pop_front();
//    }

//    if (uint8_t serial_control = cpu.bus.read(0xFF02); serial_control & 0x80) {
//        uint8_t c = cpu.bus.read(0xFF01);

//        serial_output += c;
//        serial_control &= ~0x80;
//        cpu.bus.write(0xFF02, serial_control);
//        bus.trigger_interrupt(Bus::InterruptFlag::SERIAL);
//        if (serial_output.size()% 30 == 0 )
//        {
//            serial_output += "\n";
//        }
//    }
}
