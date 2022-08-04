#pragma once

#include "bus.h"

struct CpuRegisters
{
    struct { union {
        std::uint16_t af;
        struct {
            union {
                std::uint8_t f;
                struct {
                    bool   : 4;
                    bool c : 1;
                    bool h : 1;
                    bool n : 1;
                    bool z : 1;
                } flags;
            };
            std::uint8_t a;
        };
    }; };

    struct { union {
        std::uint16_t bc;
        struct {
            std::uint8_t c;
            std::uint8_t b;
        };
    }; };

    struct { union {
        std::uint16_t de;
        struct {
            std::uint8_t e;
            std::uint8_t d;
        };
    }; };

    struct { union {
        std::uint16_t hl;
        struct {
            std::uint8_t l;
            std::uint8_t h;
        };
    }; };

    std::uint16_t sp;
    std::uint16_t pc;
};

struct Cpu
{
    CpuRegisters registers;
    Bus &bus;

    std::string last_inst_str;

    std::uint8_t arg1;
    std::uint8_t arg2;

    Cpu(Bus &bus)
        : bus(bus)
    {
        registers.a  = 0x01;
        registers.f  = 0xb0;
        registers.b  = 0x00;
        registers.c  = 0x13;
        registers.d  = 0x00;
        registers.e  = 0xd8;
        registers.h  = 0x01;
        registers.l  = 0x4d;
        registers.sp = 0xfffe;
        registers.pc = 0x100;
    }

    std::uint8_t fetch_byte() {
        return bus[registers.pc++];
    }

    std::size_t run_instruction(int opcode);
    std::size_t run_interrupts();

    std::size_t run_once();

    std::string state_str() const;
};

