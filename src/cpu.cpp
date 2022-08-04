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
std::array<std::uint8_t, Inst::usize> fetch_instruction_data(Cpu &cpu)
{
    std::array<std::uint8_t, Inst::usize> result;

    if (Inst::usize == 0)
    {
        return result;
    }

    result[0] = InstructionTraits<Inst>::opcode;
    for (size_t i=1; i<Inst::usize; ++i)
    {
        result[i] = cpu.fetch_byte();
    }

    return result;
}

template<typename Inst>
std::conditional_t<Inst::ticks==0,std::size_t,void> call_inst_return(Cpu &cpu, const std::array<std::uint8_t, Inst::usize> &data)
{
    if constexpr (Inst::size == 1)
    {
        return call(Inst{}, cpu);
    }
    else if constexpr (Inst::usize == 2)
    {
        if constexpr (Inst::signed_arg)
        {
            return call(Inst{}, cpu, std::int8_t(data[1]));
        }
        else
        {
            return call(Inst{}, cpu, data[1]);
        }
    }
    else if constexpr (Inst::size == 3)
    {
        return call(Inst{}, cpu, data[1] | data[2] << 8);
    }
    else if constexpr (Inst::size == 0)
    {
        std::ostringstream out;
        out <<  std::hex << std::uppercase << (int)Inst::opcode << " not implemented";
        throw std::runtime_error(out.str());
    }
}

template<typename Inst>
std::size_t call_ext_inst(Cpu &cpu)
{
    cpu.last_inst_str = inst_to_str(Inst{}, cpu, std::array<int,2>{ 0xCB, Inst::opcode & 0xFF });

    if constexpr (Inst::size != 0)
    {
        call(Inst{}, cpu);
        return Inst::ticks;
    }
    else
    {
        std::ostringstream out;
        out << std::hex << std::uppercase << (int)Inst::opcode << " not implemented";
        throw std::runtime_error(out.str());
    }
}

template<std::size_t N=0>
std::size_t run_extendend_instruction_helper(int opcode, Cpu &cpu)
{
    if (opcode == (N | 0xCB << 8))
    {
        //return call_ext_inst<Inst>(cpu);
        using Inst = Instruction<N | 0xCB << 8>;
        if constexpr (Inst::new_style)
            return call_inst2<Inst>(cpu);
    }

    if constexpr (N < 0xFF)
    {
        return run_extendend_instruction_helper<N+1>(opcode, cpu);
    }

    return 0;
}

template<typename Inst>
std::size_t call_inst(Cpu &cpu)
{
     auto data = fetch_instruction_data<Inst>(cpu);

     cpu.last_inst_str = inst_to_str(Inst{}, cpu,data);

     /*if constexpr (InstructionTraits<Inst>::opcode == 0xCB)
     {
         auto ext_opcode = data[1] | InstructionTraits<Inst>::opcode << 8;
         return run_extendend_instruction_helper<>(ext_opcode, cpu);
     }
     else */if constexpr (Inst::ticks > 0)
     {
         call_inst_return<Inst>(cpu, data);
         return Inst::ticks;
     }
     else
     {
         return call_inst_return<Inst>(cpu, data);
     }
}

template<typename Inst>
std::size_t call_inst_new_style(Cpu &cpu)
{
    if (InstructionTraits<Inst>::opcode == 0x18)
    {
       // std::cout << "Break\n";
    }

    auto data = fetch_instruction_data<Inst>(cpu);
    if constexpr (Inst::usize > 1)
    {
        cpu.arg1 = data[1];
    }

    if constexpr (Inst::usize > 2)
    {
        cpu.arg2= data[2];
    }

    if constexpr (InstructionTraits<Inst>::opcode == 0xCB)
    {
        auto ext_opcode = data[1] | InstructionTraits<Inst>::opcode << 8;
        return run_extendend_instruction_helper<>(ext_opcode, cpu);
    }

    cpu.last_inst_str = inst_to_str(Inst{}, cpu, data);
    return call_inst2<Inst>(cpu);
}

template<std::size_t N=0>
std::size_t run_instruction_helper(int opcode, Cpu &cpu)
{
    if (opcode == N)
    {
        using Inst = Instruction<N>;
        if constexpr (Inst::new_style)
        {
            return call_inst_new_style<Inst>(cpu);
        }
        else
        return call_inst<Inst>(cpu);
    }

    if constexpr (N < 0xFF)
    {
        return run_instruction_helper<N+1>(opcode, cpu);
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
