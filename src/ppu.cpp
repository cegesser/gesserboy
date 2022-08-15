#include "ppu.h"


#include <stdexcept>
#include <string>
#include <sstream>
#include <iostream>
#include "bus.h"


static const int LINES_PER_FRAME = 154;
static const int TICKS_PER_LINE = 456;
static const int TICKS_MODE_2 = 80;
static const int TICKS_MODE_3 = 250;
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


//456 ticks per line
//156 lines per frame
//One frame: 70224 dots @ 59.7 fps
void Ppu::run_ounce()
{
    ++line_tick;

    if (line_tick == TICKS_PER_LINE)
    {
        line_tick = 0;

        ++line_y;

        if (line_y > LINES_PER_FRAME)
        {
            line_y = 0;
        }

        lcd_status.lyc_eq_ly_flag = line_y == ly_compare;

        if (line_y == ly_compare)
        {
            emit_stat_interrupt(*this, lcd_status.STAT_lyc_interrupt_source);
        }

        if (line_y == YRES)
        {
            frame_ready = true;
            if (lcd_control.lcd_ppu_enable)
            {
                bus.interrupts.trigger_interrupt(Interrupts::VBLANK);
            }
            emit_stat_interrupt(*this, lcd_status.STAT_vblank_interrupt_source);
        }
    }


    if (line_y < YRES)
    {
        //modes 2/3/0
        if (line_tick < TICKS_MODE_2)
        {
            //Searching OAM for OBJs whose Y coordinate overlap this line

            if (line_tick == 0)
            {
                lcd_status.current_mode = Ppu::OAM;
                emit_stat_interrupt(*this, lcd_status.STAT_oam_interrupt_source);
            }


            //CPU cannot access OAM ($FE00-FE9F).            
        }
        else if (line_tick < TICKS_MODE_3) //168 to 291 depending on sprite count
        {
            //Reading OAM and VRAM to generate the picture

            if (lcd_status.current_mode == Ppu::OAM)
            {
                lcd_status.current_mode = Ppu::TRANSFER;
                render_scanline();
            }

            //CPU cannot access OAM ($FE00-FE9F).
            //CPU cannot access VRAM or CGB palette data registers ($FF69,$FF6B).
        }
        else
        {
            //Returning beam to start of line
            if (lcd_status.current_mode == Ppu::TRANSFER)
            {
                lcd_status.current_mode = Ppu::HBLANK;
                emit_stat_interrupt(*this, lcd_status.STAT_hblank_interrupt_source);
            }

        }
    }
    else
    {
        //mode 1
        //Returning beam to top of screen

        if (lcd_status.current_mode == Ppu::HBLANK)
        {
            lcd_status.current_mode = Ppu::VBLANK;
        }
    }
}

union ObjAttribs {
    std::uint8_t value;
    struct {
        std::uint8_t cgb_pallete_number : 3;
        bool cgb_tile_vram_bank : 1;
        std::uint8_t pallete_number : 1;
        bool x_flip : 1;
        bool y_flip : 1;
        bool bg_window_over : 1;//Bit7   BG and Window over OBJ (0=No, 1=BG and Window colors 1-3 over the OBJ)
//                    Bit6   Y flip          (0=Normal, 1=Vertically mirrored)
//                    Bit5   X flip          (0=Normal, 1=Horizontally mirrored)
//                    Bit4   Palette number  **Non CGB Mode Only** (0=OBP0, 1=OBP1)
//                    Bit3   Tile VRAM-Bank  **CGB Mode Only**     (0=Bank 0, 1=Bank 1)
//                    Bit2-0 Palette number  **CGB Mode Only**     (OBP0-7)
    };
};

void Ppu::render_scanline()
{
    int map_offset = lcd_control.bg_tile_map_area == 0
            ? 0x9800-0x8000
            : 0x9C00-0x8000;
    int tiles_offset = lcd_control.bg_window_tile_data_area == 0
            ? 0x9000-0x8000
            : 0x8000-0x8000;

    auto tile_pixel_value=[&](int tile_index, int x, int y, int tiles_offset)
    {
        auto tile_start = video_ram + tiles_offset + tile_index*16;

        auto b0 = tile_start[2*y];
        auto b1 = tile_start[2*y+1];

        auto mask = 1 << (7-x);
        auto pix = ((b0 & mask) ? 1 : 0)
                 | ((b1 & mask) ? 2 : 0);

        return pix;
    };

    //std::cout << std::dec << (int)line_y << " " <<  (int)lcd_scroll_x << "\n";

    for (int line_x=0; line_x < XRES; ++line_x)
    {
        auto ty = (32 + (lcd_scroll_y + line_y) / 8) % 32;
        auto tx = (32 + (lcd_scroll_x + line_x) / 8) % 32;

        int tile_index = video_ram[map_offset+tx+ty*32];

        if (lcd_control.bg_window_tile_data_area==0)
        {
            tile_index = int8_t(tile_index);
        }

        auto x_in_tile = (lcd_scroll_x+line_x)%8;
        auto y_in_tile = (lcd_scroll_y+line_y)%8;

        auto pix = tile_pixel_value(tile_index, x_in_tile, y_in_tile, tiles_offset);
        auto background_color = bg_palette_data[pix];

        screen_buffer[line_x + line_y * XRES] = background_color;
    }

    const auto sh = lcd_control.big_obj ? 16 : 8;


    int line_sprites = 0;

    for (int n = 39; n >= 0 && line_sprites<10; --n)
    {
        auto s = n*4;

        auto sy   = obj_attribute_memory[s+0] -16;
        auto sx   = obj_attribute_memory[s+1] - 8;
        auto tile = obj_attribute_memory[s+2];
        auto attribs = ObjAttribs{ obj_attribute_memory[s+3] };

        if (sy > line_y || (sy + sh) <= line_y)
        {
            continue;
        }

        ++line_sprites;

        int y_in_tile = attribs.y_flip
                ? (sh-1) - (line_y - sy)
                : line_y - sy;

        for (int x = 0; x < 8; x++)
        {
            if (sx+x < 0 || sx+x >= 160)
            {
                continue;
            }

            auto x_in_tile = attribs.x_flip
                    ? 7-x
                    : x;

            auto pix = tile_pixel_value(tile, x_in_tile, y_in_tile, 0);
            auto color = obj_palette_data[attribs.pallete_number][pix];
            if (color > 0)
            {
                screen_buffer[sx + x + line_y * XRES] = color;
            }
        }
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

