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
    bool scanline_stepping = false;
    bool frame_stepping = false;

    int mode = 0;

    System system;

    std::vector<std::pair<std::string,std::string>> log;

public:
    GesserBoy(const std::string &rom_file)
        : system(rom_file)
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
        static const float target_frame_time = 1.0f / 120.0f;
        static float s_accumulated_time = 0.0f;

        if ( ! handle_input() )
        {
            return false;
        }

        s_accumulated_time += elapsed_time;
        if (s_accumulated_time < target_frame_time)
        {
            return true;
        }

        s_accumulated_time -= target_frame_time;


        if (stepping)
        {
            step();
        }
        else if (scanline_stepping)
        {
            auto curr_line = system.ppu.line_y;
            while (curr_line == system.ppu.line_y)
            {
                step();
            }
        }
        else if (frame_stepping)
        {
            running = true;
            frame_step();
            running = false;
        }
        else if (running)
        {
            frame_step();
        }

        frame_stepping = scanline_stepping = stepping = false;

        Clear(olc::BLACK);
        {
            draw_debug_info(0, 0);
            draw_last_instructions(81, 0);

            if (mode == 1)
            {
                draw_screen(0, 9*7, 1);
                draw_tileset(164, 9*7);
            }
            else if (mode == 2)
            {
                draw_tile_map(0, 9*7);
            }
            else
            {
                draw_screen(0, 9*7, 2);
            }
        }

        return true;
    }

    bool handle_input()
    {
        if (GetKey(olc::Key::ESCAPE).bPressed)
        {
            return false;
        }
        if (GetKey(olc::Key::F5).bPressed)
        {
            running = ! running;
        }
        if (GetKey(olc::Key::F10).bPressed)
        {
            stepping = ! stepping;
            running = false;
        }
        if (GetKey(olc::Key::F11).bPressed)
        {
            scanline_stepping = ! scanline_stepping;
            running = false;
        }
        if (GetKey(olc::Key::F12).bPressed)
        {
            frame_stepping = ! frame_stepping;
            running = false;
        }

        if (GetKey(olc::Key::M).bPressed)
        {
            mode = (mode+1) % 3;
        }

        system.bus.p1_joypad.a      = GetKey(olc::Key::Z).bHeld;
        system.bus.p1_joypad.b      = GetKey(olc::Key::X).bHeld;
        system.bus.p1_joypad.up     = GetKey(olc::Key::UP).bHeld;
        system.bus.p1_joypad.down   = GetKey(olc::Key::DOWN).bHeld;
        system.bus.p1_joypad.left   = GetKey(olc::Key::LEFT).bHeld;
        system.bus.p1_joypad.right  = GetKey(olc::Key::RIGHT).bHeld;
        system.bus.p1_joypad.start  = GetKey(olc::Key::ENTER).bHeld;
        system.bus.p1_joypad.select = GetKey(olc::Key::SHIFT).bHeld;

        return true;
    }

    void step()
    {
        auto log_last_inst=[&](const std::string &inst = std::string())
        {
            log.emplace_back(""/*system.cpu.state_str()*/, inst.empty() ? system.cpu.last_inst_str : inst);
            if (log.size() > 6)
            {
                log.erase(log.begin());
            }
        };

        try
        {
            system.tick();
            log_last_inst();
        }
        catch (std::logic_error const &e)
        {
            for (const auto &line : log)
            {
                std::cout << line.first << " " << line.second << std::endl;
            }
            std::cerr << e.what() << std::endl;

            running = false;
            log_last_inst();
        }
        catch (std::exception const &e)
        {
            for (const auto &line : log)
            {
                std::cout << line.first << " " << line.second << std::endl;
            }
            std::cerr << system.cpu.last_inst_str << std::endl;
            std::cerr << system.cpu.state_str() << " | " << e.what() << std::endl;

            running = false;
            log_last_inst();
            log_last_inst(e.what());
        }
    }

    void frame_step()
    {
        while ( running && ! system.ppu.frame_ready )
        {
            step();
        }
        system.ppu.frame_ready = false;
    }

    auto color(int plt_color)
    {
        return plt_color == 3 ? olc::BLACK
             : plt_color == 2 ? olc::VERY_DARK_GREY
             : plt_color == 1 ? olc::DARK_GREY
             : plt_color == 0 ? olc::GREY
                              : olc::WHITE  ;
    }

    void draw_screen(int x_start, int y_start, int scale)
    {
        static olc::Sprite screen_area(160, 144);
        olc::Pixel *buffer = screen_area.GetData();
        auto ppu_buffer = system.ppu.screen_buffer;
        for (int i=0; i<160*144; ++i)
        {
            buffer[i] = color(ppu_buffer[i]);
        }
        DrawSprite(x_start, y_start, &screen_area, scale);
    }

    void draw_debug_info(int x_start, int y_start)
    {
        auto registers = system.cpu.registers;
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

        DrawString(x_start+8,   y_start, "A:"+to_str_hex(registers.a, 2), olc::RED);
        DrawString(x_start+5*8, y_start, flags_str, olc::RED);

        DrawString(x_start, y_start + 9*1, "BC:"+to_str_hex(registers.bc, 4), olc::RED);
        DrawString(x_start, y_start + 9*2, "DE:"+to_str_hex(registers.de, 4), olc::RED);
        DrawString(x_start, y_start + 9*3, "HL:"+to_str_hex(registers.hl, 4), olc::RED);
        DrawString(x_start, y_start + 9*4, "PC:"+to_str_hex(registers.pc, 4), olc::RED);
        DrawString(x_start, y_start + 9*5, "SP:"+to_str_hex(registers.sp, 4), olc::RED);
        DrawString(x_start, y_start + 9*6, "SX:"+std::to_string(system.ppu.lcd_scroll_x)
                                         +" SY:"+std::to_string(system.ppu.lcd_scroll_y)
                                         +" LY:"+std::to_string(system.ppu.line_y)
                                         +" LYC:"+std::to_string(system.ppu.ly_compare), olc::RED);
    }

    void draw_last_instructions(int x_start, int y_start)
    {
        for (size_t i=0; i<log.size(); ++i)
        {
            DrawString(x_start, y_start + i*9, log[i].second, olc::RED);
        }
    }

    void draw_tileset(int x_start, int y_start)
    {
        for (int t=0; t<384; ++t)
        {

            for (int y=0; y<8; ++y)
            {
                for (int x=0; x<8; ++x)
                {
                    auto pix = system.ppu.tile_pixel_value(t, x, y, 0);

                    auto color = pix == 1 ? olc::VERY_DARK_GREY
                               : pix == 2 ? olc::DARK_GREY
                               : pix == 3 ? olc::BLACK
                                          : olc::GREY;
                    Draw(x_start + x + t * 8 % 160,
                         y_start + y + t * 8 / 160 * 8, color);
                }
            }
        }
    }

    void draw_tile_map(int x_start, int y_start)
    {
        static olc::Sprite screen_area(256, 256);

        auto raw_map = system.ppu.render_tiles_map();

        olc::Pixel *buffer = screen_area.GetData();

        for (int i=0; i<256*256; ++i)
        {
            buffer[i] = color(raw_map[i]);
        }

        for (int x=0; x<160; ++x)
        {
            screen_area.SetPixel((x+system.ppu.lcd_scroll_x/8)%256, (system.ppu.lcd_scroll_y/8)%256, olc::RED);
            screen_area.SetPixel((x+system.ppu.lcd_scroll_x/8)%256, (system.ppu.lcd_scroll_y/8+144)%256, olc::RED);
        }

        for (int y=0; y<144; ++y)
        {
            screen_area.SetPixel((system.ppu.lcd_scroll_x/8)%256,     (y+system.ppu.lcd_scroll_y/8)%256, olc::RED);
            screen_area.SetPixel((system.ppu.lcd_scroll_x/8+160)%256, (y+system.ppu.lcd_scroll_y/8)%256, olc::RED);
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

    try
    {
        GesserBoy emulator(argv[1]);
        if (emulator.Construct(330, 352, 2, 2))
            emulator.Start();
    }
    catch(std::exception const &e)
    {
        std::cerr << e.what() << std::endl;
    }

    std::cout << "The end" << std::endl;
}
