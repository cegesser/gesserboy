#include "ppu.h"
#include <stdexcept>
#include <string>
#include <iostream>
#include "bus.h"

int ticks = 0;
int gpu_ticks = 0;

Ppu::Ppu(Bus &bus) : bus(bus)
{

}

void render_scanline(Ppu &ppu)
{
    int mapOffset = ppu.lcd_control.bg_tile_map_area ? 0x1c00 : 0x1800;
    mapOffset += (((ppu.scanline + ppu.lcd_scroll_y) & 255) >> 3) << 5;

    int lineOffset = (ppu.lcd_scroll_x >> 3);

    int x = ppu.lcd_scroll_x & 7;
    int y = (ppu.scanline + ppu.lcd_scroll_y) & 7;

    int pixelOffset = ppu.scanline * 160;


    unsigned short tile = (unsigned short)ppu.video_ram[mapOffset + lineOffset];
    //if((gpu.control & GPU_CONTROL_TILESET) && tile < 128) tile += 256;

    unsigned char scanlineRow[160];

    // if bg enabled
    int i;
    for(i = 0; i < 160; i++) {

        auto tile_pixel_index=[&ppu](int tile, int x, int y)
        {
            auto tile_start = ppu.video_ram + tile*16;

            auto b0 = tile_start[2*y];
            auto b1 = tile_start[2*y+1];

            auto mask = 1 << (7-x);
            auto pix = ((b0 & mask) >> (7-x) )
                     | ((b1 & mask) >> (6-x) );

            return pix;
        };
        auto color_index = tile_pixel_index(tile, x, y);

        scanlineRow[i] = color_index;

        ppu.screen_buffer[pixelOffset] = color_index;
        pixelOffset++;

        x++;
        if(x == 8) {
            x = 0;
            lineOffset = (lineOffset + 1) & 31;
            tile = ppu.video_ram[mapOffset + lineOffset];
            //if((gpu.control & GPU_CONTROL_TILESET) && tile < 128) tile += 256;
        }
    }
/*
    // if sprites enabled
    for(i = 0; i < 40; i++) {
        struct sprite sprite = ((struct sprite *)oam)[i];

        int sx = sprite.x - 8;
        int sy = sprite.y - 16;

        if(sy <= gpu.scanline && (sy + 8) > gpu.scanline) {
            COLOUR *pal = spritePalette[sprite.palette];

            int pixelOffset = gpu.scanline * 160 + sx;


            unsigned char tileRow;
            if(sprite.vFlip) tileRow = 7 - (gpu.scanline - sy);
            else tileRow = gpu.scanline - sy;

            int x;
            for(x = 0; x < 8; x++) {
                if(sx + x >= 0 && sx + x < 160 && (~sprite.priority || !scanlineRow[sx + x])) {
                    unsigned char colour;

                    if(sprite.hFlip) colour = tiles[sprite.tile][tileRow][7 - x];
                    else colour = tiles[sprite.tile][tileRow][x];

                    if(colour) {
                        framebuffer[pixelOffset].r = pal[colour].r;
                        framebuffer[pixelOffset].g = pal[colour].g;
                        framebuffer[pixelOffset].b = pal[colour].b;

                    }

                    pixelOffset++;
                }
            }
        }
    }
*/
}

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
                bus.int_trigger(Bus::int_VBLANK);

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

            render_scanline(*this);

            gpu_ticks -= 172;
        }

        break;
    }
}



uint8_t Ppu::read(uint16_t address) const
{
    switch (address)
    {
        case 0xFF40: return lcd_control.value;
        case 0xFF41: return lcd_status;
        case 0xFF42: return lcd_scroll_y;
        case 0xFF43: return lcd_scroll_x;
        case 0xFF44: return scanline;
        case 0xFF45: return ly_compare;

        case 0xFF46: //DMA


        case 0xFF47:// - BGP (BG Palette Data) (R/W) - Non CGB Mode Only
        case 0xFF48: // - OBP0 (OBJ Palette 0 Data) (R/W), FF49 - OBP1 (OBJ Palette 1 Data) (R/W) - Both Non CGB Mode Only
        case 0xFF49: // - ^^^^^
            break;
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
        case 0xFF40: lcd_control.value = value; return;
        case 0xFF41: lcd_status = value; return;
        case 0xFF42: lcd_scroll_y = value; return;
        case 0xFF43: lcd_scroll_x = value; return;
        case 0xFF45: ly_compare = value; return;
        case 0xFF46: //DMA
        {
            std::cout << "DMA: " << int(value) << "\n";
            auto dst = 0xfe00;
            auto src = value << 8;
            auto len = 160;

            for (int i = 0; i < len; i++) {
                bus[dst+i] = std::uint8_t(bus[src+i]);
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
