#include "ppu.h"


#include <stdexcept>
#include <string>
#include <iostream>
#include "bus.h"


Ppu::Ppu(Bus &bus) : bus(bus)
{

}

int ticks = 0;
int gpu_ticks = 0;


void Ppu::run_ounce()
{
    ++ticks;

    static int old_ticks = 0;

    gpu_ticks += ticks - old_ticks;

    old_ticks = ticks;

    switch(mode)
    {
    case HBLANK:
        if (gpu_ticks >= 204)
        {
            scanline++;

            if (scanline == 143)
            {
                bus.trigger_interrupt(Bus::InterruptFlag::VBLANK);

                mode = VBLANK;
            }
            else
            {
                mode = OAM;
            }

            gpu_ticks -= 204;
        }
        break;

    case VBLANK:
        if (gpu_ticks >= 456)
        {
            scanline++;

            if (scanline == 144)
            {
                frame_ready = true;
            }

            if(scanline > 153)
            {
                scanline = 0;
                mode = OAM;
            }

            gpu_ticks -= 456;
        }
        break;

    case OAM:
        if (gpu_ticks >= 80)
        {
            mode = VRAM;
            gpu_ticks -= 80;
        }
        break;

    case VRAM:
        if (gpu_ticks >= 172)
        {
            mode = HBLANK;

            //render_scanline(*this);

            gpu_ticks -= 172;
        }

        break;
    }
}



uint8_t Ppu::read(uint16_t address) const
{
    switch (address)
    {
        case 0xFF40:
            std::cout << "PPU read: FF40 " << std::hex << (int)lcd_control.value << std::endl;
            return lcd_control.value;
        case 0xFF41: std::cout << "PPU read: " << std::hex << address << std::endl;return lcd_status;
        case 0xFF42: std::cout << "PPU read: " << std::hex << address << std::endl;return lcd_scroll_y;
        case 0xFF43: std::cout << "PPU read: " << std::hex << address << std::endl;return lcd_scroll_x;
        case 0xFF44: return scanline;
        case 0xFF45: std::cout << "PPU read: " << std::hex << address << std::endl;return ly_compare;

        case 0xFF46: //DMA
            //if (dma.active)
            {
                return 0xFF;
            }

        //return ppu_oam_read(address);


        case 0xFF47:// - BGP (BG Palette Data) (R/W) - Non CGB Mode Only
        case 0xFF48: // - OBP0 (OBJ Palette 0 Data) (R/W), FF49 - OBP1 (OBJ Palette 1 Data) (R/W) - Both Non CGB Mode Only
        case 0xFF49: // - ^^^^^
            break;
        case 0xFF4A: std::cout << "PPU read: " << std::hex << address << std::endl;return window_y_pos;
        case 0xFF4B: std::cout << "PPU read: " << std::hex << address << std::endl;return window_x_pos;
    }

    throw std::runtime_error("PPU read from " + std::to_string(address));
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
    switch (address)
    {
        case 0xFF40: lcd_control.value = value; return;
        case 0xFF41: lcd_status = value; std::cout << "PPU write: " << std::hex << address << " " << int(value) << std::endl; return;
        case 0xFF42: lcd_scroll_y = value; return;
        case 0xFF43: lcd_scroll_x = value; return;
        case 0xFF45: ly_compare = value; return;
        case 0xFF46: //DMA
        {
            auto src = int(value) << 8;
            auto dst = 0xfe00;
            std::cout << "DMA starting: " << std::hex << src << "-" << (src+160) << " to " << dst << "\n";
            auto len = 160;

             for (int i = 0; i < len; i++) {
                 bus.write(dst+i, bus.read(src+i));
             }
            return;
        }
        case 0xFF47: write_palette_data(bg_palette_data, value); return;
        case 0xFF48: write_palette_data(obj_palette_data[0], value); return;
        case 0xFF49: write_palette_data(obj_palette_data[1], value); return;
        case 0xFF4A: window_y_pos = value; return;
        case 0xFF4B: window_x_pos = value; return;
    }

    throw std::runtime_error("PPU write to " + std::to_string(address));
}
