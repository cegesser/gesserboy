#include "ppu.h"
#include <stdexcept>
#include <string>
#include "bus.h"

Ppu::Ppu(Bus &bus) : bus(bus)
{

}

struct {

    int tick = 0;
} gpu;

int ticks = 0;;

void Ppu::run_ounce()
{
    ++ticks;

    static int lastTicks = 0;

    gpu.tick += ticks - lastTicks;

    lastTicks = ticks;

    switch(mode)
    {
    case HBLANK:
        if (gpu.tick >= 204)
        {
            scanline++;

            if (scanline == 143)
            {
                bus.int_trigger(Bus::int_VBLANK);

                mode = VBLANK;
            }
            else
            {
                mode = OAM;
            }

            gpu.tick -= 204;
        }
        break;

    case VBLANK:
        if (gpu.tick >= 456)
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

            gpu.tick -= 456;
        }
        break;

    case OAM:
        if (gpu.tick >= 80)
        {
            mode = VRAM;
            gpu.tick -= 80;
        }
        break;

    case VRAM:
        if (gpu.tick >= 172)
        {
            mode = HBLANK;

            //renderScanline();

            gpu.tick -= 172;
        }

        break;
    }
}

uint8_t Ppu::read(uint16_t address) const
{
    switch (address)
    {
        case 0xFF40: return lcd_control;
        case 0xFF41: return lcd_status;
        case 0xFF42: return lcd_scroll_y;
        case 0xFF43: return lcd_scroll_x;
        case 0xFF44: return scanline; // /*std::cout << "xxxx";*/ return 0x94; return bus.lcd_line_y;
        case 0xFF45: return ly_compare;

        case 0xFF46: //DMA


        case 0xFF47:// - BGP (BG Palette Data) (R/W) - Non CGB Mode Only
        case 0xFF48: // - OBP0 (OBJ Palette 0 Data) (R/W), FF49 - OBP1 (OBJ Palette 1 Data) (R/W) - Both Non CGB Mode Only
        case 0xFF49: // - ^^^^^

        case 0xFF4A: return window_y_pos;
        case 0xFF4B: return window_x_pos;
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
        case 0xFF40: lcd_control = value; return;
        case 0xFF41: lcd_status = value; return;
        case 0xFF42: lcd_scroll_y = value; return;
        case 0xFF43: lcd_scroll_x = value; return;
        case 0xFF45: ly_compare = value; return;
        case 0xFF46: //DMA
//        {
//            auto dst = 0xfe00;
//            auto src = value << 8;
//            auto len = 160;

//            for (int i = 0; i < len; i++) bus_write(dst + i, bus_read(src + i));
//            return;
//        }
        case 0xFF47: write_palette_data(bg_palette_data, value); return;
        case 0xFF48: write_palette_data(obj_palette_data[0], value); return;
        case 0xFF49: write_palette_data(obj_palette_data[1], value); return;
        case 0xFF4A: window_y_pos = value; return;
        case 0xFF4B: window_x_pos = value; return;
    }

    throw std::runtime_error("PPU write to " + std::to_string(address));
}
