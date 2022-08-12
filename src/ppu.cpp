#include "ppu.h"


#include <stdexcept>
#include <string>
#include <sstream>
#include <iostream>
#include "bus.h"


static const int LINES_PER_FRAME = 154;
static const int TICKS_PER_LINE = 456;
static const int YRES = 144;
static const int XRES = 160;

Ppu::Ppu(Bus &bus) : bus(bus)
{

}

void emit_stat_interrupt(Ppu &ppu, bool condition)
{
    if (condition && ppu.lcd_control.lcd_ppu_enable)
    {
        ppu.bus.interrupts.trigger_interrupt(Interrupts::LCDSTAT);
    }
}


//456 dots per line
//156 lines per frame
//One frame: 70224 dots @ 59.7 fps
void Ppu::run_ounce()
{
    ++line_tick;

    if (line_tick == 456)
    {
        line_tick = 0;

        ++line_y;
        lcd_status.lyc_eq_ly_flag = line_y == ly_compare;

        if (line_y > 154)
        {
            line_y = 0;
            lcd_status.lyc_eq_ly_flag = line_y == ly_compare;

            if (lcd_control.lcd_ppu_enable)
            {
                bus.interrupts.trigger_interrupt(Interrupts::VBLANK);
            }
            emit_stat_interrupt(*this, lcd_status.STAT_vblank_interrupt_source);
        }

        if (line_y == ly_compare && lcd_status.STAT_lyc_interrupt_source)
        {
            emit_stat_interrupt(*this, lcd_status.STAT_lyc_interrupt_source);
        }

        if (line_y == 144)
        {
            frame_ready = true;
        }
    }


    if (line_y < 144)
    {
        //modes 2/3/0
        if (line_tick < 80)
        {
            if (line_tick == 0 && lcd_status.STAT_oam_interrupt_source)
            {
                emit_stat_interrupt(*this, lcd_status.STAT_oam_interrupt_source);
            }
            lcd_status.current_mode = Ppu::OAM;
            //CPU cannot access OAM ($FE00-FE9F).

            //Searching OAM for OBJs whose Y coordinate overlap this line
        }
        else if (line_tick < 205) //168 to 291 depending on sprite count
        {
            lcd_status.current_mode = Ppu::TRANSFER;
            //CPU cannot access OAM ($FE00-FE9F).
            //CPU cannot access VRAM or CGB palette data registers ($FF69,$FF6B).

            //Reading OAM and VRAM to generate the picture
        }
        else
        {
            if (lcd_status.current_mode == Ppu::TRANSFER)
            {
                emit_stat_interrupt(*this, lcd_status.STAT_hblank_interrupt_source);
            }

            lcd_status.current_mode = Ppu::HBLANK;
        }
    }
    else
    {
        //mode 1
        lcd_status.current_mode = Ppu::VBLANK;
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
    //std::cout << "DMA starting: " << std::hex << src << "-" << (src+160) << " to " << dst << "\n";
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
