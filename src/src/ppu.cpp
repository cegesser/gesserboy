#include "ppu.h"


#include <stdexcept>
#include <string>
#include <iostream>
#include "bus.h"


Ppu::Ppu(Bus &bus) : bus(bus)
{

}

struct sprite {
        unsigned char y;
        unsigned char x;
        unsigned char tile;
        struct {
                unsigned char priority : 1;
                unsigned char vFlip : 1;
                unsigned char hFlip : 1;
                unsigned char palette : 1;
        };
};

int ticks = 0;
int gpu_ticks = 0;

void render_scanline(Ppu &ppu)
{
    //return;
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

    int base_mapOffset = ppu.lcd_control.bg_tile_map_area ? 0x1c00 : 0x1800;



    int mapOffset = base_mapOffset + ((((ppu.scanline + ppu.lcd_scroll_y) & 255) >> 3) << 5);


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

//    // if sprites enabled
//    for(i = 0; i < 40; i++) {
//        struct sprite sprite = ((struct sprite *)ppu.obj_attribute_memory)[i];

//        int sx = sprite.x - 8;
//        int sy = sprite.y - 16;

//        if(sy <= ppu.scanline && (sy + 8) > ppu.scanline) {
//            auto *pal = ppu.obj_palette_data[sprite.palette];

//            int pixelOffset = ppu.scanline * 160 + sx;


//            unsigned char tileRow;
//            if(sprite.vFlip) tileRow = 7 - (ppu.scanline - sy);
//            else tileRow = ppu.scanline - sy;

//            int x;
//            for(x = 0; x < 8; x++) {
//                if(sx + x >= 0 && sx + x < 160 && (~sprite.priority || !scanlineRow[sx + x])) {
//                    unsigned char colour;

//                    unsigned short tile = (unsigned short)ppu.video_ram[sprite.tile + lineOffset];


//                    if(sprite.hFlip) colour = tile_pixel_index(tile, 7-x, tileRow);
//                    else             colour = tile_pixel_index(tile, x, tileRow);

//                    if(colour) {
//                        ppu.screen_buffer[pixelOffset] = colour;

//                    }

//                    pixelOffset++;
//                }
//            }
//        }
//    }

}

struct {
    bool active = false;
    uint8_t byte;
    uint8_t value;
    uint8_t start_delay;
} dma;

void dma_start(uint8_t start) {
    dma.active = true;
    dma.byte = 0;
    dma.start_delay = 2;
    dma.value = start;
}

void Ppu::run_ounce()
{
    ++ticks;
/*
    if (ticks % 4)
    {
        if (dma.active)
        {
            if (dma.start_delay)
            {
                --dma.start_delay;
            }
            else
            {
                uint8_t v = bus[dma.value * 0x100 + dma.byte];
                bus[dma.byte + 0xFE00] = v;

                dma.byte++;

                dma.active = dma.byte < 0xA0;
            }
        }
    }
*/
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
        case 0xFF40:
            std::cout << "PPU read: FF40 " << std::hex << (int)lcd_control.value << std::endl;
            return lcd_control.value;
        case 0xFF41: std::cout << "PPU read: " << std::hex << address << std::endl;return lcd_status;
        case 0xFF42: std::cout << "PPU read: " << std::hex << address << std::endl;return lcd_scroll_y;
        case 0xFF43: std::cout << "PPU read: " << std::hex << address << std::endl;return lcd_scroll_x;
        case 0xFF44: return scanline;
        case 0xFF45: std::cout << "PPU read: " << std::hex << address << std::endl;return ly_compare;

        case 0xFF46: //DMA
            if (dma.active) {
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
//            if ( dma.active )
//            {
//                std::cout << "DMA ongoing: " << int(value) << "\n";

//            }
//            if ( ! dma.active )
//            {
//                std::cout << "DMA starting: " << int(value) << "\n";
//                dma_start(value);
//            }
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
