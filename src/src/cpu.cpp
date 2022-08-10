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

template<typename Inst>
typename std::size_t call(Cpu &cpu)
{
    using Impl = typename Inst::impl_type;

    if constexpr (std::is_same_v<Impl,NotImplemented>)
    {
        std::ostringstream out;
        out << "Not implemented: ";
        out << std::hex << std::setw(2) << std::uppercase << std::setfill('0') << int(opcode_v<Inst>);
        throw std::runtime_error(out.str());
    }

    if constexpr (Inst::ticks == 0)
    {
        return Impl::execute(cpu);
    }
    else
    {
        Impl::execute(cpu);
        return Inst::ticks;
    }
}

template<std::size_t N=0xCB00>
std::size_t run_extendend_instruction_helper(uint16_t opcode, Cpu &cpu)
{
    switch (opcode)
    {
        case N+0x0: return call<Instruction<N+0x0>>(cpu);
        case N+0x1: return call<Instruction<N+0x1>>(cpu);
        case N+0x2: return call<Instruction<N+0x2>>(cpu);
        case N+0x3: return call<Instruction<N+0x3>>(cpu);
        case N+0x4: return call<Instruction<N+0x4>>(cpu);
        case N+0x5: return call<Instruction<N+0x5>>(cpu);
        case N+0x6: return call<Instruction<N+0x6>>(cpu);
        case N+0x7: return call<Instruction<N+0x7>>(cpu);
        case N+0x8: return call<Instruction<N+0x8>>(cpu);
        case N+0x9: return call<Instruction<N+0x9>>(cpu);
        case N+0xA: return call<Instruction<N+0xA>>(cpu);
        case N+0xB: return call<Instruction<N+0xB>>(cpu);
        case N+0xC: return call<Instruction<N+0xC>>(cpu);
        case N+0xD: return call<Instruction<N+0xD>>(cpu);
        case N+0xE: return call<Instruction<N+0xE>>(cpu);
        case N+0xF: return call<Instruction<N+0xF>>(cpu);
    }

    if constexpr (N+0x10 < 0xCBFF)
    {
        return run_extendend_instruction_helper<N+0x10>(opcode, cpu);
    }

    static_assert (0xCB00 <= N && N < 0xCBFF);
    return 0;
}

template<typename Inst>
std::size_t fetch_data_and_call(Cpu &cpu)
{
    using Impl = typename Inst::impl_type;
    Impl::fetch(cpu);

    if constexpr (opcode_v<Inst> == 0xCB)
    {
        auto ext_opcode = cpu.arg1 | 0xCB << 8;
        return run_extendend_instruction_helper<>(ext_opcode, cpu);
    }

    cpu.last_inst_str = inst_to_str(Inst{}, cpu);
    return call<Inst>(cpu);
}

template<std::size_t N=0>
std::size_t run_instruction_helper(uint8_t opcode, Cpu &cpu)
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

    if constexpr (N+0x10 < 0xFF)
    {
        return run_instruction_helper<N+0x10>(opcode, cpu);
    }

    static_assert (N < 0xFF);
    return 0;
}

uint8_t Cpu::fetch_byte() {
    return bus.read(registers.pc++);
}

size_t Cpu::run_once()
{
    size_t ticks = 0;


    if ( halted )
    {
        ticks += 4;

        const auto interrupts_triggered = bus.read(0xFF0F);
        if (interrupts_triggered)
        {
            halted = false;
        }
    }
    else
    {
        const uint8_t opcode = fetch_byte();

        ticks += run_instruction_helper<>(opcode, *this);
    }

    return ticks;
}


size_t Cpu::run_interrupts()
{
    if ( ! bus.interrupts_master_enable_flag )
    {
        return 0;
    }

    const auto interrupts_triggered = bus.read(0xFF0F);
    const auto interrupts_enabled = bus.read(0xFFFF);
    const auto interrupts_engaged = interrupts_enabled & interrupts_triggered;

    if ( ! interrupts_engaged ) {
        return 0;
    }

    static constexpr struct {
        Bus::InterruptFlag interrupt;
        std::uint16_t address;
    } handlers[] = {
        { Bus::InterruptFlag::VBLANK,  0x40 },
        { Bus::InterruptFlag::LCDSTAT, 0x48 },
        { Bus::InterruptFlag::TIMER,   0x50 },
        { Bus::InterruptFlag::SERIAL,  0x58 },
        { Bus::InterruptFlag::JOYPAD,  0x60 }
    };

    for (const auto &handler : handlers)
    {
        auto flag = int(handler.interrupt);
        if (interrupts_engaged & flag) {
            bus.interrupts_master_enable_flag = false;
            bus.write(0xFF0F, interrupts_triggered & ~flag);

//            if (registers.pc == 0x1fff)
//            {
//                throw std::logic_error("");
//            }

            PUSH<PC>::execute(*this);
            registers.pc = handler.address;
            return 12;
        }
    }

    return 0;
}