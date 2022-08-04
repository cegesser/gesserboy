#pragma once

#include <cstdint>

struct Bus;

class Ppu
{
public:
    enum Mode {
            HBLANK = 0,
            VBLANK = 1,
            OAM    = 2,
            VRAM   = 3,
    };

    Ppu(Bus &bus);

    void run_ounce();

    std::uint8_t read(std::uint16_t address) const;
    void write(std::uint16_t address, std::uint8_t value);

    Bus &bus;

    Mode mode = HBLANK;



    // 0x8000 - 0x97FF : CHR RAM
    std::uint8_t video_ram[0x2000];

    //FF40
    union {
        std::uint8_t value;
        struct {
            bool lcd_ppu_enable : 1;
            std::uint8_t window_tile_map_area : 1; //0=9800-9BFF, 1=9C00-9FFF
            bool window_enable : 1;
            std::uint8_t bg_window_tile_map_area : 1; //0=8800-97FF, 1=8000-8FFF
            std::uint8_t bg_tile_map_area : 1; //0=9800-9BFF, 1=9C00-9FFF
            std::uint8_t obj_size : 1; //0=8x8, 1=8x16
            bool obj_enable : 1;
            bool bg_window_priority : 1;
        };
    } lcd_control;

    //FF41
    //Bit 6 - LYC=LY STAT Interrupt source         (1=Enable) (Read/Write)
    //Bit 5 - Mode 2 OAM STAT Interrupt source     (1=Enable) (Read/Write)
    //Bit 4 - Mode 1 VBlank STAT Interrupt source  (1=Enable) (Read/Write)
    //Bit 3 - Mode 0 HBlank STAT Interrupt source  (1=Enable) (Read/Write)
    //Bit 2 - LYC=LY Flag                          (0=Different, 1=Equal) (Read Only)
    //Bit 1-0 - Mode Flag                          (Mode 0-3, see below) (Read Only)
    //          0: HBlank
    //          1: VBlank
    //          2: Searching OAM
    //          3: Transferring Data to LCD Controller
    std::uint8_t lcd_status = 0;

    //FF42
    std::uint8_t lcd_scroll_y = 0;
    //FF43
    std::uint8_t lcd_scroll_x = 0;
    //FF44
    std::uint8_t scanline = 0;

    //FF45
    std::uint8_t ly_compare = 0;

    //0xFF47
    std::uint8_t bg_palette_data[4];
    //0xFF48
    //0xFF49
    std::uint8_t obj_palette_data[2][4];

    //0xFF4A - WY (Window Y Position)
    std::uint8_t window_y_pos = 0;
    //0xFF4B - WX (Window X Position + 7)
    std::uint8_t window_x_pos = 0;

    bool frame_ready = false;

    std::uint8_t screen_buffer[160*256];
};

