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
    bool running = false;
    bool stepping = false;

    System gboy;

    std::vector<std::string> log;


public:
    GesserBoy()
        //: gboy("roms/Tetris.gb")
        // : gboy("roms/Zelda.gb")
        //: gboy("roms/Star Wars.gb")
         : gboy("roms/cpu_instrs.gb")
        //: gboy("roms/individual/06-ld r,r.gb")
        //: gboy("roms/individual/07-jr,jp,call,ret,rst.gb")
    {
        // Name your application
        sAppName = "GesserBoy";

        //System gboy("roms/Zelda.gb");
        //System gboy("roms/Star Wars.gb");
        //System gboy("roms/cpu_instrs.gb");
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

//                    if (gboy.cpu.registers.pc == 0x02a0 )
//                    {
//                        running = false;
//                        break;
//                    }
                }

                gboy.ppu.frame_ready = false;
                //render
            }
        }
        catch (std::logic_error const &e)
        {
            std::cerr << e.what() << std::endl;
            running = false;
            log_last_inst();
        }
        catch (std::exception const &e)
        {
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

            //Draw tileset
            const auto y_start = 9*7;
            for (int t=0; t<384; ++t)
            {
                auto tile_start = gboy.ppu.video_ram + t*16;
                for (int n=0; n<16; n+=2)
                {
                    auto b0 = tile_start[n];
                    auto b1 = tile_start[n+1];

                    for (int x=0; x<8; ++x)
                    {
                        auto mask = 1 << (7-x);
                        auto pix = ((b0 & mask) >> (7-x) )
                                 | ((b1 & mask) >> (6-x) );

                        if (pix == 0) continue;
                        auto color = pix == 2 ? olc::DARK_GREY
                                   : pix == 3 ? olc::GREY
                                   : pix == 1 ? olc::VERY_DARK_GREY
                                              : olc::WHITE;
                        auto y = n/2;
                        Draw((t * 8 % 160) + x,
                             y_start + (y + t * 8 / 160 * 8), color);
                    }
                }
            }
            DrawString(0, 9*6, "Dbg: ["+gboy.serial_output + "]", olc::RED);
        }

        return true;
    }
};



int main(int argc, char**argv)
{
    GesserBoy emulator;
    if (emulator.Construct(256, 240+16, 4, 4))
        emulator.Start();
}


