#include "cpu.h"

#include "instructions.h"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <array>

std::string Cpu::state_str() const
{
    std::ostringstream out;
    out << "PC: " << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << registers.pc << " A: "
        << std::setw(2) << std::setfill('0') << int(registers.a)  << " BC: "
        << std::setw(4) << std::setfill('0') << int(registers.bc) << " DE: "
        << std::setw(4) << std::setfill('0') << int(registers.de) << " HL: "
        << std::setw(4) << std::setfill('0') << int(registers.hl);

    std::string flags_str = " znhc";
    if ( ! registers.flags.z) flags_str[1] = '-';
    if ( ! registers.flags.n) flags_str[2] = '-';
    if ( ! registers.flags.h) flags_str[3] = '-';
    if ( ! registers.flags.c) flags_str[4] = '-';

    return out.str() + flags_str;
}


template<typename Inst>
std::string inst_to_str(const Inst &inst, const Cpu &cpu)
{
    std::ostringstream out;

    if constexpr (Inst::size == 0)
    {
        return "";
    }

    auto opcode = opcode_v<Inst>;

    out << "[" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << int(opcode);
    if constexpr (Inst::size > 1)
    {
        out << " " << std::setw(2) << std::setfill('0') << int(cpu.arg1);
    }
    else
    {
        out << "   ";
    }
    if constexpr (Inst::size > 2)
    {
        out << " " << std::setw(2) << std::setfill('0') << int(cpu.arg2);
    }
    else
    {
        out << "   ";
    }

    out << "] ";

    using Impl = typename Inst::impl_type;
    out << Impl::mnemonic(cpu);

    return out.str();
}

template<std::size_t N=0>
std::size_t run_extendend_instruction_helper(int opcode, Cpu &cpu)
{
    if (opcode == (N | 0xCB << 8))
    {
        using Inst = Instruction<N | 0xCB << 8>;
        if constexpr (Inst::new_style)
            return call<Inst>(cpu);
    }

    if constexpr (N < 0xFF)
    {
        return run_extendend_instruction_helper<N+1>(opcode, cpu);
    }

    return 0;
}


template<typename Inst>
std::size_t fetch_data_and_call(Cpu &cpu)
{
    if (opcode_v<Inst> == 0x18)
    {
       // std::cout << "Break\n";
    }

    if constexpr (Inst::size > 1)
    {
        cpu.arg1 = cpu.fetch_byte();
    }

    if constexpr (Inst::size > 2)
    {
        cpu.arg2= cpu.fetch_byte();
    }

    if constexpr (opcode_v<Inst> == 0xCB)
    {
        auto ext_opcode = cpu.arg1 | opcode_v<Inst> << 8;
        return run_extendend_instruction_helper<>(ext_opcode, cpu);
    }

    cpu.last_inst_str = inst_to_str(Inst{}, cpu);
    return call<Inst>(cpu);
}

template<std::size_t N=0>
std::size_t run_instruction_helper(int opcode, Cpu &cpu)
{
    switch (opcode)
    {
        case N+0x0: return fetch_data_and_call<Instruction<N+0x0>>(cpu);
        case N+0x1: return fetch_data_and_call<Instruction<N+0x1>>(cpu);
        case N+0x2: return fetch_data_and_call<Instruction<N+0x2>>(cpu);
        case N+0x3: return fetch_data_and_call<Instruction<N+0x3>>(cpu);
        case N+0x4: return fetch_data_and_call<Instruction<N+0x4>>(cpu);
        case N+0x5: return fetch_data_and_call<Instruction<N+0x5>>(cpu);
        case N+0x6: return fetch_data_and_call<Instruction<N+0x6>>(cpu);
        case N+0x7: return fetch_data_and_call<Instruction<N+0x7>>(cpu);
        case N+0x8: return fetch_data_and_call<Instruction<N+0x8>>(cpu);
        case N+0x9: return fetch_data_and_call<Instruction<N+0x9>>(cpu);
        case N+0xA: return fetch_data_and_call<Instruction<N+0xA>>(cpu);
        case N+0xB: return fetch_data_and_call<Instruction<N+0xB>>(cpu);
        case N+0xC: return fetch_data_and_call<Instruction<N+0xC>>(cpu);
        case N+0xD: return fetch_data_and_call<Instruction<N+0xD>>(cpu);
        case N+0xE: return fetch_data_and_call<Instruction<N+0xE>>(cpu);
        case N+0xF: return fetch_data_and_call<Instruction<N+0xF>>(cpu);
    }

    if constexpr (N < 0xFF)
    {
        return run_instruction_helper<N+0x10>(opcode, cpu);
    }

    return 0;
}

std::size_t Cpu::run_instruction(int opcode)
{
    return run_instruction_helper<>(opcode, *this);
}

size_t Cpu::run_once()
{
    size_t ticks = 0;


    const uint8_t opcode = fetch_byte();

    ticks += run_instruction(opcode);

    ticks += run_interrupts();

    return ticks;
}


size_t Cpu::run_interrupts()
{
    if ( ! bus.interrupts_master_enable_flag || ! bus.interrupts_enable || ! bus.interrupts_flags) {
        return 0;
    }

    auto engaged = bus.interrupts_enable & bus.interrupts_flags;

    auto fire=[&](Bus::IntType interrupt, std::uint16_t address)
    {
        if (engaged & interrupt) {
            bus.interrupts_flags &= ~interrupt;
            bus.interrupts_master_enable_flag = false;
            op_push(*this, registers.pc);
            registers.pc = address;
            return 12;
        }
        return 0;
    };

    if (auto ticks = fire(Bus::int_VBLANK, 0x40))
    {
        return ticks;
    }
    if (auto ticks = fire(Bus::int_LCDSTAT, 0x48))
    {
        return ticks;
    }
    if (auto ticks = fire(Bus::int_TIMER, 0x50))
    {
        return ticks;
    }
    if (auto ticks = fire(Bus::int_SERIAL, 0x58))
    {
        return ticks;
    }
    if (auto ticks = fire(Bus::int_JOYPAD, 0x60))
    {
        return ticks;
    }

    return 0;
}
