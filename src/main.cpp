#include <iostream>
#include <memory>

//---------------
#include "system.h"

//----------------
#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

// Override base class with your custom functionality
class GesserBoy : public olc::PixelGameEngine
{
    bool running = true;
    bool stepping = false;

    System gboy;

    std::vector<std::string> log;


public:
    GesserBoy()
        : gboy("roms/Tetris.gb")
        //: gboy("roms/Dr. Mario.gb")
        //: gboy("roms/Metroid II.gb")
        //: gboy("roms/Zelda.gb")
        //: gboy("roms/Star Wars.gb")
        //: gboy("roms/cpu_instrs.gb")
        //: gboy("roms/individual/01-special.gb")
        //: gboy("roms/individual/02-interrupts.gb") //passed
        //: gboy("roms/individual/03-op sp,hl.gb") //
        //: gboy("roms/individual/04-op r,imm.gb") //passed
        //: gboy("roms/individual/05-op rp.gb") //passed
        //: gboy("roms/individual/06-ld r,r.gb") //passed
        //: gboy("roms/individual/07-jr,jp,call,ret,rst.gb")  //passed
        //: gboy("roms/individual/08-misc instrs.gb") //passed
        //: gboy("roms/individual/09-op r,r.gb") //passed
        //: gboy("roms/individual/10-bit ops.gb") //passed
        //: gboy("roms/individual/11-op a,(hl).gb")
    {
        // Name your application
        sAppName = "GesserBoy";
    }

public:
    bool OnUserCreate() override
    {
        // Called once at the start, so create things here
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
                //render
            }
        }
        catch (std::logic_error const &e)
        {
            for (const auto &line : gboy.log)
            {
                std::cout << line << std::endl;
            }
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
            std::cerr << gboy.cpu.last_inst_str << std::endl;
            std::cerr << gboy.cpu.state_str() << " | " << e.what() << std::endl;
            running = false;
            log_last_inst();
            log_last_inst(e.what());
        }

        stepping = false;

        static const float target_frame_time = 1.0f / 60.0f;
        static float fAccumulatedTime = 0.0f;

        //fAccumulatedTime = elapsed_time;
        fAccumulatedTime += elapsed_time;
        if (fAccumulatedTime >= target_frame_time)
        {
            //std::cout << fAccumulatedTime << " Render" << '\n';
            fAccumulatedTime -= target_frame_time;

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
            }


            //Draw instructions
            for (size_t i=0; i<log.size(); ++i)
            {
                DrawString(81, i*9, log[i], olc::RED);
            }



            //Draw screen
            auto draw_screen = [this](int x_start, int y_start)
            {
                for (int y=0; y<144; ++y)
                {
                    for (int x=0; x<160; ++x)
                    {
                        auto pix = gboy.ppu.screen_buffer[x + 160*y];

                        //if (pix == 0) continue;
                        auto color = pix == 3 ? olc::GREY
                                   : pix == 2 ? olc::DARK_GREY
                                   : pix == 1 ? olc::VERY_DARK_GREY
                                              : olc::BLACK;
                        Draw(x + x_start, y + y_start, color);
                    }
                }
            };

//            //Draw tileset
            auto draw_tileset = [this](int x_start, int y_start)
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
            };

            const auto x_start = 0;
            const auto y_start = 9*7;

            auto draw_tile_map = [this](int x_start, int y_start)
            {
                static olc::Sprite screen_area(256, 256);

                int map_offset = gboy.ppu.lcd_control.bg_tile_map_area ? 0x1c00 : 0x1800;

                for (int ty=0; ty<32; ++ty) for (int tx=0; tx<32; ++tx)
                {
                    auto x_pos = tx*8;
                    auto y_pos = ty*8;
                    auto tile_index = gboy.ppu.video_ram[map_offset+tx+ty*32];

                    auto tile_pixel_value=[&ppu=gboy.ppu](int tile, int x, int y)
                    {
                        auto tile_start = ppu.video_ram + tile*16;

                        auto b0 = tile_start[2*y];
                        auto b1 = tile_start[2*y+1];

                        auto mask = 1 << (7-x);
                        auto pix = ((b0 & mask) >> (7-x) )
                                 | ((b1 & mask) >> (6-x) );

                        return pix;
                    };
                    for (int y=0; y<8; ++y) for (int x=0; x<8; ++x)
                    {
                        auto pix = tile_pixel_value(tile_index, x, y);


                        //if (pix == 0) continue;
                        auto color = pix == 0 ? olc::BLACK
                                   : pix == 1 ? olc::VERY_DARK_GREY
                                   : pix == 2 ? olc::DARK_GREY
                                   : pix == 3 ? olc::GREY
                                              : olc::WHITE  ;

                        screen_area.GetData()[x_pos + x + (y_pos + y)*256] = color;
                    }
                }
                DrawSprite(x_start, y_start, &screen_area);
            };

            //draw_screen(x_start, y_start);
            //draw_tileset(x_start, y_start);
            draw_tile_map(x_start, y_start);

            //DrawString(0, 9*6, "Dbg: ["+gboy.serial_output + "]", olc::RED);
//            std::ostringstream out;
//            out << std::dec << (int)gboy.ppu.scanline;
//            DrawString(0, 9*6, out.str(), olc::RED);
        }

        return true;
    }
};



int main(int argc, char**argv)
{
    GesserBoy emulator;
    if (emulator.Construct(260+16, 240*1.4, 2, 2))
        emulator.Start();
}


