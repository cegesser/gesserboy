#include <iostream>
#include <memory>

//---------------
#include "system.h"

//----------------
#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

class GesserBoy : public olc::PixelGameEngine
{
    bool running = true;
    bool stepping = false;

    System gboy;

    std::vector<std::string> log;


public:
    GesserBoy(const std::string &rom_file)
        : gboy(rom_file)
    {
        sAppName = "GesserBoy";
    }

public:
    bool OnUserCreate() override
    {
        return true;
    }

    bool OnUserUpdate(float elapsed_time) override
    {
        if (GetKey(olc::Key::F5).bPressed)
        {
            running = ! running;
        }
        if (GetKey(olc::Key::F10).bPressed)
        {
            stepping = ! stepping;
            running = false;
        }
        gboy.bus.p1_joypad.a      = GetKey(olc::Key::Z).bHeld;
        gboy.bus.p1_joypad.b      = GetKey(olc::Key::X).bHeld;
        gboy.bus.p1_joypad.up     = GetKey(olc::Key::UP).bHeld;
        gboy.bus.p1_joypad.down   = GetKey(olc::Key::DOWN).bHeld;
        gboy.bus.p1_joypad.left   = GetKey(olc::Key::LEFT).bHeld;
        gboy.bus.p1_joypad.right  = GetKey(olc::Key::RIGHT).bHeld;
        gboy.bus.p1_joypad.start  = GetKey(olc::Key::ENTER).bHeld;
        gboy.bus.p1_joypad.select = GetKey(olc::Key::SHIFT).bHeld;

        static const float target_frame_time = 1.0f / 60.0f;
        static float s_accumulated_time = 0.0f;

        s_accumulated_time += elapsed_time;
        if (s_accumulated_time < target_frame_time)
        {
            return true;
        }

        s_accumulated_time -= target_frame_time;

        auto log_last_inst = [&](const std::string &inst = std::string())
        {
            log.push_back(inst.empty() ? gboy.cpu.last_inst_str : inst);
            if (log.size() > 6)
            {
                log.erase(log.begin());
            }
        };

        try
        {
            if (stepping)
            {
                gboy.tick();
                log_last_inst();
            }
            else if (running)
            {
                while ( ! gboy.ppu.frame_ready )
                {
                    gboy.tick();
                    log_last_inst();
                }
                gboy.ppu.frame_ready = false;
            }
        }
        catch (std::logic_error const &e)
        {
            for (const auto &line : gboy.log)
            {
                std::cout << line << std::endl;
            }
            gboy.log.clear();
            std::cerr << e.what() << std::endl;
            running = false;
            log_last_inst();
        }
        catch (std::exception const &e)
        {
            for (const auto &line : gboy.log)
            {
                std::cout << line << std::endl;
            }
            gboy.log.clear();
            std::cerr << gboy.cpu.last_inst_str << std::endl;
            std::cerr << gboy.cpu.state_str() << " | " << e.what() << std::endl;
            running = false;
            log_last_inst();
            log_last_inst(e.what());
        }

        stepping = false;
        {
            this->Clear(olc::BLACK);
            //Draw CPU
            {
                auto registers = gboy.cpu.registers;
                std::string flags_str = " znhc";
                if ( ! registers.flags.z) flags_str[1] = '-';
                if ( ! registers.flags.n) flags_str[2] = '-';
                if ( ! registers.flags.h) flags_str[3] = '-';
                if ( ! registers.flags.c) flags_str[4] = '-';

                auto to_str_hex = [](std::uint16_t value, int w=4)
                {
                    std::ostringstream out;
                    out << std::hex << std::uppercase << std::setw(w) << std::setfill('0') << value;
                    return out.str();
                };

                DrawString(8, 0, "A:"+to_str_hex(registers.a, 2), olc::RED);
                DrawString(5*8, 0, flags_str, olc::RED);

                DrawString(0, 9*1, "BC:"+to_str_hex(registers.bc, 4), olc::RED);
                DrawString(0, 9*2, "DE:"+to_str_hex(registers.de, 4), olc::RED);
                DrawString(0, 9*3, "HL:"+to_str_hex(registers.hl, 4), olc::RED);
                DrawString(0, 9*4, "PC:"+to_str_hex(registers.pc, 4), olc::RED);
                DrawString(0, 9*5, "SP:"+to_str_hex(registers.sp, 4), olc::RED);
                DrawString(0, 9*6, "SX:"+std::to_string(gboy.ppu.lcd_scroll_x)
                                 +" SY:"+std::to_string(gboy.ppu.lcd_scroll_y)
                                 +" LYC:"+std::to_string(gboy.ppu.ly_compare), olc::RED);
            }

            //Draw instructions
            for (size_t i=0; i<log.size(); ++i)
            {
                DrawString(81, i*9, log[i], olc::RED);
            }

            const auto x_start = 0;
            const auto y_start = 9*7;            

            //draw_tileset(x_start, y_start);
            //draw_tile_map(x_start, y_start);
            draw_screen(x_start, y_start);
        }

        return true;
    }

    void draw_screen(int x_start, int y_start)
    {
        static olc::Sprite screen_area(160, 144);
        olc::Pixel *buffer = screen_area.GetData();
        auto ppu_buffer = gboy.ppu.screen_buffer;
        for (int i=0; i<160*144; ++i)
        {
            auto plt_color = ppu_buffer[i];
            auto color = plt_color == 3 ? olc::BLACK
                       : plt_color == 2 ? olc::VERY_DARK_GREY
                       : plt_color == 1 ? olc::DARK_GREY
                       : plt_color == 0 ? olc::GREY
                                        : olc::WHITE  ;
            buffer[i] = color;
        }
        DrawSprite(x_start, y_start, &screen_area);
    }

    void draw_tileset(int x_start, int y_start)
    {
        for (int t=0; t<384; ++t)
        {
            auto tile_pixel_index=[&ppu=gboy.ppu](int tile, int x, int y)
            {
                auto tile_start = ppu.video_ram + tile*16;

                auto b0 = tile_start[2*y];
                auto b1 = tile_start[2*y+1];

                auto mask = 1 << (7-x);
                auto pix = ((b0 & mask) >> (7-x) )
                         | ((b1 & mask) >> (6-x) );

                return pix;
            };

            for (int y=0; y<8; ++y)
            {
                for (int x=0; x<8; ++x)
                {
                    auto pix = tile_pixel_index(t, x, y);

                    if (pix == 0) continue;
                    auto color = pix == 2 ? olc::DARK_GREY
                               : pix == 3 ? olc::GREY
                               : pix == 1 ? olc::VERY_DARK_GREY
                                          : olc::WHITE;
                    Draw((t * 8 % 160) + x,
                         y_start + (y + t * 8 / 160 * 8), color);
                }
            }
        }
    }

    void draw_tile_map(int x_start, int y_start)
    {
        static olc::Sprite screen_area(256, 256);

        union ObjAttribs {
            std::uint8_t value;
            struct {
                std::uint8_t cgb_pallete_number : 3;
                bool cgb_tile_vram_bank : 1;
                std::uint8_t pallete_number : 1;
                bool x_flip : 1;
                bool y_flip : 1;
                bool bg_window_over : 1;

//                    Bit7   BG and Window over OBJ (0=No, 1=BG and Window colors 1-3 over the OBJ)
//                    Bit6   Y flip          (0=Normal, 1=Vertically mirrored)
//                    Bit5   X flip          (0=Normal, 1=Horizontally mirrored)
//                    Bit4   Palette number  **Non CGB Mode Only** (0=OBP0, 1=OBP1)
//                    Bit3   Tile VRAM-Bank  **CGB Mode Only**     (0=Bank 0, 1=Bank 1)
//                    Bit2-0 Palette number  **CGB Mode Only**     (OBP0-7)
            };
        };

        auto draw_tile = [&gboy=gboy](int x_pos, int y_pos, int tile_index, int tiles_offset, bool obj, ObjAttribs attribs)
        {
            auto tile_pixel_value=[&](int tile_index, int x, int y)
            {
                auto tile_start = gboy.ppu.video_ram + tiles_offset + tile_index*16;

                auto b0 = tile_start[2*y];
                auto b1 = tile_start[2*y+1];

                auto mask = 1 << (7-x);
                auto pix = ((b0 & mask) >> (7-x) )
                         | ((b1 & mask) >> (6-x) );

                return pix;
            };
            for (int y=0; y<8; ++y) for (int x=0; x<8; ++x)
            {
                if (x_pos + x < 0 || x_pos + x > 255
                 || y_pos + y < 0 || y_pos + y > 255) continue;

                auto pix = tile_pixel_value(tile_index, x, y);
                auto plt_color = gboy.ppu.bg_palette_data[pix];
                if ( obj )
                {
                    if (attribs.x_flip && attribs.y_flip)
                    {
                        pix = tile_pixel_value(tile_index, 8-x, 8-y);
                    }
                    else if (attribs.x_flip)
                    {
                        pix = tile_pixel_value(tile_index, 8-x, y);
                    }
                    else if (attribs.y_flip)
                    {
                        pix = tile_pixel_value(tile_index, x, 8-y);
                    }

                    plt_color = gboy.ppu.obj_palette_data[attribs.pallete_number][pix];
                }

                auto color = plt_color == 3 ? olc::BLACK
                           : plt_color == 2 ? olc::VERY_DARK_GREY
                           : plt_color == 1 ? olc::DARK_GREY
                           : plt_color == 0 ? olc::GREY
                                            : olc::WHITE  ;

                if ( ! obj || plt_color > 0 || attribs.bg_window_over )
                {
                    screen_area.GetData()[x_pos + x + (y_pos + y)*256] = color;
                }
            }
        };

        int map_offset = gboy.ppu.lcd_control.bg_tile_map_area == 0
                ? 0x9800-0x8000
                : 0x9C00-0x8000;
        int tiles_offset = gboy.ppu.lcd_control.bg_window_tile_data_area == 0
                ? 0x9000-0x8000
                : 0x8000-0x8000;

        for (int ty=0; ty<32; ++ty) for (int tx=0; tx<32; ++tx)
        {
            auto x_pos = (tx*8 - gboy.ppu.lcd_scroll_x + 256)%256;
            auto y_pos = (ty*8 - gboy.ppu.lcd_scroll_y + 256)%256;
            int tile_index = gboy.ppu.video_ram[map_offset+tx+ty*32];
            if (gboy.ppu.lcd_control.bg_window_tile_data_area==0)
            {
                tile_index = int8_t(tile_index);
            }

            draw_tile(x_pos, y_pos, tile_index, tiles_offset, false, {0});
        }

        for (int s=0; s<40*4; s+=4)
        {
            auto sy = gboy.ppu.obj_attribute_memory[s+0];
            auto sx = gboy.ppu.obj_attribute_memory[s+1];
            auto tile = gboy.ppu.obj_attribute_memory[s+2];
            ObjAttribs attribs;
            attribs.value = gboy.ppu.obj_attribute_memory[s+3];

            draw_tile(sx-8, sy-16, tile, 0, true, attribs);

            if (gboy.ppu.lcd_control.big_obj)
            {
                draw_tile(sx-8, sy-8, tile+1, 0, true, attribs);
            }

        }

        DrawSprite(x_start, y_start, &screen_area);
    };
};

int main(int argc, char**argv)
{
    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " ROM-File" << std::endl;
        return EXIT_SUCCESS;
    }

    GesserBoy emulator(argv[1]);
    if (emulator.Construct(260+16, 240, 4, 4))
        emulator.Start();

    std::cout << "The end" << std::endl;
}
