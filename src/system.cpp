#include "system.h"
#include <iostream>



System::System(const std::string &cartridge_filename)
    : cart(cartridge_filename)
    , bus{ ppu, cart }
    , cpu{ bus }
    , ppu{ bus }
{
    print_header(cart.header);
}

void System::tick()
{
    static std::size_t cycles = 0;

    //std::cout << cpu.state_str() + " | " ;
    auto ticks = cpu.run_once();
    cycles += ticks/4;
    for (size_t i=0; i<ticks; ++i)
    {
        bus.run_timer_once();
        ppu.run_ounce();
    }
    //std::cout << std::dec << cycles << " " <<  cpu.last_inst_str;


    if (uint8_t serial_control = cpu.bus[0xFF02]; serial_control & 0x80) {
        uint8_t c = cpu.bus[0xFF01];

        serial_output += c;
        serial_control &= ~0x80;
        cpu.bus[0xFF02] = serial_control;
        bus.int_trigger(Bus::int_SERIAL);

    }
//    std::cout <<  "\t" ;
//    for (int i=cpu.registers.sp; i< 0xcfff /* 1<<16*/; ++i)
//    {
//        std::cout << (int)(uint8_t)cpu.bus[i] << " ";
//    }

    //std::cout << '\n';
}
