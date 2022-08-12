#include "ppu.h"


#include <stdexcept>
#include <string>
#include <sstream>
#include <iostream>
#include "bus.h"


Ppu::Ppu(Bus &bus) : bus(bus)
{

}

int cpu_ticks = 0;
int ppu_ticks = 0;


void Ppu::run_ounce()
{
    ++cpu_ticks;

    static int old_cpu_ticks = 0;

    ppu_ticks += cpu_ticks - old_cpu_ticks;

    old_cpu_ticks = cpu_ticks;

    switch(lcd_status.mode_flag)
    {
    case HBLANK:
        if (ppu_ticks >= 204)
        {
            line_y++;
            lcd_status.lyc_eq_ly_flag = line_y == ly_compare;

            if (line_y == 143)
            {
                if (lcd_control.lcd_ppu_enable)
                {
                    bus.interrupts.trigger_interrupt(Interrupts::VBLANK);
                }

                lcd_status.mode_flag = VBLANK;
            }
            else
            {
                lcd_status.mode_flag = OAM;
            }

            ppu_ticks -= 204;
        }
        break;

    case VBLANK:
        if (ppu_ticks >= 456)
        {
            line_y++;
            lcd_status.lyc_eq_ly_flag = line_y == ly_compare;

            if (line_y == 144)
            {
                frame_ready = true;
            }

            if(line_y > 153)
            {
                line_y = 0;
                lcd_status.mode_flag = OAM;
            }

            ppu_ticks -= 456;
        }
        break;

    case OAM:
        if (ppu_ticks >= 80)
        {
            lcd_status.mode_flag = VRAM;
            ppu_ticks -= 80;
        }
        break;

    case VRAM:
        if (ppu_ticks >= 172)
        {
            lcd_status.mode_flag = HBLANK;

            //render_scanline(*this);

            ppu_ticks -= 172;
        }

        break;
    }
}



uint8_t Ppu::read(uint16_t address) const
{
    switch (address)
    {
        case 0xFF40: return lcd_control.value;
        case 0xFF41: return lcd_status.value;
        case 0xFF42: return lcd_scroll_y;
        case 0xFF43: return lcd_scroll_x;
        case 0xFF44: return line_y;
        case 0xFF45: return ly_compare;
        case 0xFF46: return 0xFF; //DMA
        case 0xFF47: break;// - BGP (BG Palette Data) (R/W) - Non CGB Mode Only
        case 0xFF48: break;// - OBP0 (OBJ Palette 0 Data) (R/W), FF49 - OBP1 (OBJ Palette 1 Data) (R/W) - Both Non CGB Mode Only
        case 0xFF49: break;// - ^^^^^
        case 0xFF4A: return window_y_pos;
        case 0xFF4B: return window_x_pos;
    }

    std::ostringstream out;
    out <<  "PPU read from " << std::hex << address << " not yet implemented";
    throw std::runtime_error(out.str());
}

void start_dma(Bus &bus, std::uint8_t value)
{
    auto src = int(value) << 8;
    auto dst = 0xfe00;
    std::cout << "DMA starting: " << std::hex << src << "-" << (src+160) << " to " << dst << "\n";
    auto len = 160;

     for (int i = 0; i < len; i++) {
         bus.write(dst+i, bus.read(src+i));
     }
}

void write_palette_data(uint8_t data[4], uint8_t value)
{
    for(int i = 0; i < 4; i++)
    {
        data[i] = (value >> (i * 2)) & 3;
    }
}

void Ppu::write(uint16_t address, uint8_t value)
{
    if (0xFE00 <= address  && address <= 0xFE9F)
    {
        obj_attribute_memory[address - 0xFE00] = value;
        return;
    }

    switch (address)
    {
        case 0xFF40: lcd_control.value = value; return;
        case 0xFF41: lcd_status.value = value; return;
        case 0xFF42: lcd_scroll_y = value; return;
        case 0xFF43: lcd_scroll_x = value; return;
        case 0xFF44: break; //line_y is read-only;
        case 0xFF45: ly_compare = value; return;
        case 0xFF46: start_dma(bus, value); return;
        case 0xFF47: write_palette_data(bg_palette_data, value); return;
        case 0xFF48: write_palette_data(obj_palette_data[0], value); return;
        case 0xFF49: write_palette_data(obj_palette_data[1], value); return;
        case 0xFF4A: window_y_pos = value; return;
        case 0xFF4B: window_x_pos = value; return;
    }

    std::ostringstream out;
    out <<  "PPU write to " << std::hex << address << " not yet implemented";
    throw std::runtime_error(out.str());
}
