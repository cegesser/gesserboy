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
static const int LCD_HEIGHT = 144;
static const int LCD_WIDTH = 160;

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

        if (line_y == LINES_PER_FRAME)
        {
            line_y = 0;
        }
    }

    lcd_status.lyc_eq_ly_flag = line_y == ly_compare;

    if (line_y == ly_compare)
    {
        emit_stat_interrupt(*this, lcd_status.STAT_lyc_interrupt_source);
    }


    if (line_y < LCD_HEIGHT)
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

            if (lcd_status.current_mode != Ppu::TRANSFER)
            {
                lcd_status.current_mode = Ppu::TRANSFER;
                render_current_scanline();
            }

            //CPU cannot access OAM ($FE00-FE9F).
            //CPU cannot access VRAM or CGB palette data registers ($FF69,$FF6B).
        }
        else
        {
            //Returning beam to start of line
            if (lcd_status.current_mode != Ppu::HBLANK)
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

        if (lcd_status.current_mode != Ppu::VBLANK)
        {
            frame_ready = true;

            if (lcd_control.lcd_ppu_enable)
            {
                bus.interrupts.trigger_interrupt(Interrupts::VBLANK);
            }

            emit_stat_interrupt(*this, lcd_status.STAT_vblank_interrupt_source);

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


int Ppu::tile_index_on_map(int map_offset, int tile_x, int tile_y)
{
    int tile_index = video_ram[map_offset+tile_x+tile_y*32];

    if (lcd_control.bg_window_tile_data_area==0)
    {
        tile_index = int8_t(tile_index);
    }

    return tile_index;
}

int Ppu::tile_pixel_value(int tile_index, int x, int y, int tiles_offset)
{
    auto tile_start = video_ram + tiles_offset + tile_index*16;

    auto b0 = tile_start[2*y];
    auto b1 = tile_start[2*y+1];

    auto mask = 1 << (7-x);
    auto pix = ((b0 & mask) ? 1 : 0)
             | ((b1 & mask) ? 2 : 0);

    return pix;
}

void Ppu::render_current_scanline()
{
    std::uint8_t scanline[LCD_WIDTH] = {0};

    //Draw background
    if (lcd_control.bg_window_enable)
    {
        int map_offset = lcd_control.bg_tile_map_area == 0
                ? 0x9800-0x8000
                : 0x9C00-0x8000;

        auto scrolled_y = line_y + lcd_scroll_y;
        auto y_on_map = scrolled_y % 256;
        auto tile_y = y_on_map / 8;
        auto y_in_tile = scrolled_y % 8;

        for (int line_x=0; line_x < LCD_WIDTH; ++line_x)
        {
            auto scrolled_x = line_x + lcd_scroll_x;
            auto x_on_map = scrolled_x % 256;
            auto tile_x = x_on_map / 8;
            auto x_in_tile = scrolled_x % 8;

            auto tile_index = tile_index_on_map(map_offset, tile_x, tile_y);

            auto pix = tile_pixel_value(tile_index, x_in_tile, y_in_tile, tiles_offset());
            scanline[line_x] = pix;

            auto background_color = bg_palette_data[pix];

            screen_buffer[line_x + line_y * LCD_WIDTH] = background_color;
        }
    }

    //Draw window
    if (lcd_control.bg_window_enable && lcd_control.window_enable)
    {
        int map_offset = lcd_control.window_tile_map_area == 0
                ? 0x9800-0x8000
                : 0x9C00-0x8000;

        auto window_y = int(line_y) - window_y_pos;
        if (0 <= window_y  && window_y < LCD_HEIGHT)
        {
            auto tile_y = window_y / 8;
            auto y_in_tile = window_y % 8;

            for (int line_x=0; line_x < LCD_WIDTH; ++line_x)
            {
                auto window_x = int(line_x) + window_x_pos - 7;

                if (window_x < 0 || window_x >= LCD_WIDTH)
                {
                    continue;
                }

                auto tile_x = window_x / 8;
                auto x_in_tile = window_x % 8;

                auto tile_index = tile_index_on_map(map_offset, tile_x, tile_y);

                auto pix = tile_pixel_value(tile_index, x_in_tile, y_in_tile, tiles_offset());
                auto background_color = bg_palette_data[pix];

                screen_buffer[line_x + line_y * LCD_WIDTH] = background_color;
            }
        }
    }

    if (lcd_control.obj_enable)
    {
        const auto obj_height = lcd_control.big_obj ? 16 : 8;

        for (int n = 39; n >= 0; --n)
        {
            auto s = n*4;

            auto obj_y   = obj_attribute_memory[s+0] -16;
            auto obj_x   = obj_attribute_memory[s+1] - 8;
            auto obj_tile = obj_attribute_memory[s+2];
            auto obj_attr = ObjAttribs{ obj_attribute_memory[s+3] };

            if (obj_y > line_y || (obj_y + obj_height) <= line_y)
            {
                continue;
            }

            int y_in_tile = obj_attr.y_flip
                    ? (obj_height-1) - (line_y - obj_y)
                    : line_y - obj_y;

            for (int x = 0; x < 8; x++)
            {
                auto line_x = obj_x+x;
                if (line_x< 0 || line_x >= LCD_WIDTH)
                {
                    continue;
                }

                auto x_in_tile = obj_attr.x_flip
                        ? 7-x
                        : x;

                auto pix = tile_pixel_value(obj_tile, x_in_tile, y_in_tile, 0);
                if (pix > 0)
                {
                    auto color = obj_palette_data[obj_attr.pallete_number][pix];

                    if ( ! obj_attr.bg_window_over || scanline[line_x] == 0)
                    {
                        screen_buffer[line_x + line_y * LCD_WIDTH] = color;
                    }
                }
            }
        }
    }
}

std::array<uint8_t, 256*256> Ppu::render_tiles_map()
{
    std::array<uint8_t, 256*256> screen_buffer {0};
    std::array<uint8_t, 256*256> raw_bg_plt_idx {0};

    std::uint8_t scanline[LCD_WIDTH] = {0};

    auto draw_tile = [&](int x_pos, int y_pos, int tile_index, int tiles_offset, bool obj, ObjAttribs attribs)
    {
        for (int y=0; y<8; ++y) for (int x=0; x<8; ++x)
        {
            if (x_pos + x < 0 || x_pos + x > 255
                    || y_pos + y < 0 || y_pos + y > 255) continue;

            auto pix = tile_pixel_value(tile_index, x, y, tiles_offset);

            auto buffer_index = x_pos + x + (y_pos + y)*256;
            if ( ! obj )
            {
                raw_bg_plt_idx[buffer_index] = pix;
                auto plt_color = bg_palette_data[pix];
                screen_buffer[buffer_index] = plt_color;
            }
            else if ( raw_bg_plt_idx[buffer_index] == 0 || ! attribs.bg_window_over )
            {
                if (attribs.x_flip && attribs.y_flip)
                {
                    pix = tile_pixel_value(tile_index, 7-x, 7-y, tiles_offset);
                }
                else if (attribs.x_flip)
                {
                    pix = tile_pixel_value(tile_index, 7-x, y, tiles_offset);
                }
                else if (attribs.y_flip)
                {
                    pix = tile_pixel_value(tile_index, x, 7-y, tiles_offset);
                }

                if (pix > 0)
                {
                    auto plt_color = obj_palette_data[attribs.pallete_number][pix];
                    screen_buffer[buffer_index] = plt_color;
                }
            }
        }
    };

    int map_offset = lcd_control.bg_tile_map_area == 0
            ? 0x9800-0x8000
            : 0x9C00-0x8000;

    //Draw background
    for (int ty=0; ty<32; ++ty) for (int tx=0; tx<32; ++tx)
    {
        auto x_pos = (tx*8 - lcd_scroll_x + 256)%256;
        auto y_pos = (ty*8 - lcd_scroll_y + 256)%256;
        int tile_index = video_ram[map_offset+tx+ty*32];
        if (lcd_control.bg_window_tile_data_area==0)
        {
            tile_index = int8_t(tile_index);
        }

        draw_tile(x_pos, y_pos, tile_index, tiles_offset(), false, {0});
    }

    //Draw sprites
    for (int s=0; s<40*4; s+=4)
    {
        auto sy = obj_attribute_memory[s+0];
        auto sx = obj_attribute_memory[s+1];
        auto tile = obj_attribute_memory[s+2];
        ObjAttribs attribs;
        attribs.value = obj_attribute_memory[s+3];

        draw_tile(sx-8, sy-16, tile, 0, true, attribs);

        if (lcd_control.big_obj)
        {
            draw_tile(sx-8, sy-8, tile+1, 0, true, attribs);
        }

    }

    return screen_buffer;
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
        case 0xFF47: return bg_palette_data.value;
        case 0xFF48: return obj_palette_data[0].value;
        case 0xFF49: return obj_palette_data[1].value;
        case 0xFF4A: return window_y_pos;
        case 0xFF4B: return window_x_pos;
    }

    if (0xFE00 <= address  && address <= 0xFE9F)
    {
        return obj_attribute_memory[address - 0xFE00];
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
        case 0xFF47: bg_palette_data.value = value; return;
        case 0xFF48: obj_palette_data[0].value = value; return;
        case 0xFF49: obj_palette_data[1].value = value; return;
        case 0xFF4A: window_y_pos = value; return;
        case 0xFF4B: window_x_pos = value; return;
    }

    std::ostringstream out;
    out <<  "PPU write to " << std::hex << address << " not yet implemented";
    throw std::runtime_error(out.str());
}


