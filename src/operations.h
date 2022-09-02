#pragma once

#include "cpu.h"
#include <array>
#include <iomanip>
#include <sstream>

template<typename T, T CpuRegisters::*Ptr, char MN1, char MN2=0>
struct Register {
    using data_type = T;
    static constexpr std::uint8_t size = 0;

    static data_type get(const Cpu &cpu) {
        return cpu.registers.*Ptr;
    }

    static void put(Cpu &cpu, data_type value) {
        cpu.registers.*Ptr = value;
    }

    static void print_op(std::ostream &out, const Cpu &) {
        out << MN1 << MN2;
    }
};

struct A : Register<std::uint8_t, &CpuRegisters::a, 'A'> {};
struct B : Register<std::uint8_t, &CpuRegisters::b, 'B'> {};
struct C : Register<std::uint8_t, &CpuRegisters::c, 'C'> {};
struct D : Register<std::uint8_t, &CpuRegisters::d, 'D'> {};
struct E : Register<std::uint8_t, &CpuRegisters::e, 'E'> {};
struct H : Register<std::uint8_t, &CpuRegisters::h, 'H'> {};
struct L : Register<std::uint8_t, &CpuRegisters::l, 'L'> {};

struct AF : Register<std::uint16_t, &CpuRegisters::af, 'A', 'F'> {};
struct BC : Register<std::uint16_t, &CpuRegisters::bc, 'B', 'C'> {};
struct DE : Register<std::uint16_t, &CpuRegisters::de, 'D', 'E'> {};
struct HL : Register<std::uint16_t, &CpuRegisters::hl, 'H', 'L'> {};
struct PC : Register<std::uint16_t, &CpuRegisters::pc, 'P', 'C'> {};
struct SP : Register<std::uint16_t, &CpuRegisters::sp, 'S', 'P'> {};

struct Immediate16
{
    using data_type = std::uint16_t;
    static constexpr std::uint8_t size = sizeof(data_type);

    static data_type get(const Cpu &cpu)
    {
        auto lsb = cpu.arg1;
        auto msb = cpu.arg2;
        return lsb | msb << 8;
    }

    static void print_op(std::ostream &out, const Cpu &cpu) {
        out << std::hex << std::setw(4) << std::uppercase << std::setfill('0') << std::uint16_t(cpu.arg1 | cpu.arg2 << 8);
    }
};

template<typename T>
struct Immediate8
{
    using data_type = T;
    static constexpr std::uint8_t size = sizeof (data_type);

    static data_type get(const Cpu &cpu)
    {
        return cpu.arg1;
    }

    static void print_op(std::ostream &out, const Cpu &cpu) {
        out << std::hex << std::setw(2) << std::uppercase << std::setfill('0') << int(cpu.arg1);
    }
};

using d16 = Immediate16;
using a16 = Immediate16;
struct a8 : Immediate8<std::uint8_t> {};
struct d8 : Immediate8<std::uint8_t> {};
struct r8 : Immediate8<std::int8_t> {};

struct SP_r8
{
    using data_type = std::uint16_t;
    static constexpr std::uint8_t size = 1;

    static data_type get(Cpu &cpu)
    {
        auto r8_value = r8::get(cpu);
        cpu.registers.f = 0;
        cpu.registers.flags.h = (cpu.registers.sp & 0xF) + (r8_value & 0xF) >= 0x10;
        cpu.registers.flags.c = (cpu.registers.sp & 0xFF) + (r8_value & 0xFF) >= 0x100;
        return cpu.registers.sp + r8_value;
    }

    static void print_op(std::ostream &out, const Cpu &cpu) {
        out << "SP+";
        r8::print_op(out, cpu);
    }
};

struct FlagNotZ
{
    static bool passes(const Cpu &cpu)
    {
        return ! cpu.registers.flags.z;
    }

    static std::string to_string()
    {
        return "NZ, ";
    }
};

struct FlagZ
{
    static bool passes(const Cpu &cpu)
    {
        return cpu.registers.flags.z;
    }

    static std::string to_string()
    {
        return "Z, ";
    }
};

struct FlagNotC
{
    static bool passes(const Cpu &cpu)
    {
        return ! cpu.registers.flags.c;
    }

    static std::string to_string()
    {
        return "NC, ";
    }
};

struct FlagC
{
    static bool passes(const Cpu &cpu)
    {
        return cpu.registers.flags.c;
    }

    static std::string to_string()
    {
        return "C, ";
    }
};

struct Always
{
    static bool passes(const Cpu &cpu)
    {
        return true;
    }

    static std::string to_string()
    {
        return "";
    }
};

template <typename F>
struct FlagTraits
{
    using result_type = std::size_t;
    static constexpr bool variable = true;
};

template <>
struct FlagTraits<Always>
{
    using result_type = void;
    static constexpr bool variable = false;
};

template<typename Loc, char Op=0>
struct At
{
    using data_type = std::uint8_t;
    static constexpr std::uint8_t size = Loc::size;
    //static_assert (sizeof(typename Loc::data_type) == 2);

    static data_type get(Cpu &cpu)
    {
        std::uint16_t address = Loc::get(cpu);
        if constexpr (sizeof(typename Loc::data_type) == 1)
        {
            address += 0xFF00;
        }
        if constexpr (Op == '-')
        {
            Loc::put(cpu, address-1);
        }
        if constexpr (Op == '+')
        {
            Loc::put(cpu, address+1);
        }
        return cpu.bus.read(address);
    }

    static void put(Cpu &cpu, data_type value)
    {
        std::uint16_t address = Loc::get(cpu);
        if constexpr (sizeof(typename Loc::data_type) == 1)
        {
            address += 0xFF00;
        }
        if constexpr (Op == '-')
        {
            Loc::put(cpu, address-1);
        }
        if constexpr (Op == '+')
        {
            Loc::put(cpu, address+1);
        }

        cpu.bus.write(address, value);
    }

    static void print_op(std::ostream &out, const Cpu &cpu) {

        out << "(";

        if constexpr (sizeof(typename Loc::data_type) == 1)
        {
            out << "FF00+";
            Loc::print_op(out, cpu);
        }
        else if constexpr (Op == '-')
        {
            Loc::print_op(out, cpu);
            out << "-";
        }
        else if constexpr (Op == '+')
        {
            Loc::print_op(out, cpu);
            out << "+";
        }
        else
        {
            Loc::print_op(out, cpu);
        }

        out << ")";

    }
};

template<typename Loc>
struct At16
{
    using data_type = std::uint16_t;
    static constexpr std::uint8_t size = Loc::size;

    static_assert (sizeof(typename Loc::data_type) == 2);

    static data_type get(Cpu &cpu)
    {
        std::uint16_t address = Loc::get(cpu);
        return cpu.bus.read16(address);
    }

    static void put(Cpu &cpu, data_type value)
    {
        std::uint16_t address = Loc::get(cpu);
        cpu.bus.write16(address, value);
    }

    static void print_op(std::ostream &out, const Cpu &cpu) {
        out << "(";
        Loc::print_op(out, cpu);
        out << ")";
    }
};


template<std::size_t Size>
struct Operation
{
    static constexpr std::uint8_t size = Size;

    static void fetch(Cpu &cpu)
    {
        if constexpr (Size > 1)
        {
            cpu.arg1 = cpu.fetch_byte();
        }

        if constexpr (Size > 2)
        {
            cpu.arg2= cpu.fetch_byte();
        }
    }
};

struct INVALID : Operation<1> {
    using result_type = void;

    static void execute(Cpu &) { throw std::runtime_error("Invalid instruction"); }
    static void print(std::ostream &out, const Cpu &) { out << "INVALID"; }
};

struct NOP : Operation<1> {
    using result_type = void;

    static void execute(Cpu &) { }
    static void print(std::ostream &out, const Cpu &) { out << "NOP"; }
};

struct HALT : Operation<1> {
    using result_type = void;

    static void execute(Cpu &cpu) { cpu.halted = true; }
    static void print(std::ostream &out, const Cpu &) { out << "HALT"; }
};

struct STOP : Operation<2> {
    using result_type = void;

    static void execute(Cpu &) { /*TODO*/ }
    static void print(std::ostream &out, const Cpu &) { out << "STOP"; }
};

/// Disables the master interrupt flag
struct DI : Operation<1> {
    using result_type = void;

    static void execute(Cpu &cpu) {
        cpu.inerrupts_master_enable_flag = false;
    }
    static void print(std::ostream &out, const Cpu &) { out << "DI"; }
};

/// Enables the master interrupt flag
struct EI : Operation<1> {
    using result_type = void;

    static void execute(Cpu &cpu) {
        cpu.inerrupts_master_enable_flag = true;
    }
    static void print(std::ostream &out, const Cpu &) { out << "EI"; }
};

/// Sets the carry flag
struct SCF : Operation<1> {
    using result_type = void;

    static void execute(Cpu &cpu) {
        cpu.registers.flags.n = false;
        cpu.registers.flags.h = false;
        cpu.registers.flags.c = true;
    }
    static void print(std::ostream &out, const Cpu &) { out << "SCF"; }
};

/// Flips the carry flag
struct CCF : Operation<1> {
    using result_type = void;

    static void execute(Cpu &cpu) {
        cpu.registers.flags.n = false;
        cpu.registers.flags.h = false;
        cpu.registers.flags.c = ! cpu.registers.flags.c;
    }
    static void print(std::ostream &out, const Cpu &) { out << "CCF"; }
};

struct PREFIX : Operation<2> {
    using result_type = std::size_t;

    static result_type execute(Cpu &) { return 0; }
    static void print(std::ostream &out, const Cpu &) { out << "PREFIX"; }
};

template<typename Dst, typename Src>
struct LD : Operation<1+Dst::size+Src::size>
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        auto value = Src::get(cpu);
        Dst::put(cpu, value);
    }

    static void print(std::ostream &out, const Cpu &cpu) {
        out << "LD ";
        Dst::print_op(out, cpu);
        out << ", ";
        Src::print_op(out, cpu);
    }
};

template<typename Cond, typename Loc>
struct JP : Operation<1+Loc::size>
{
    using result_type = typename FlagTraits<Cond>::result_type;

    static result_type execute(Cpu &cpu) {
        if (Cond::passes(cpu))
        {
            auto address = Loc::get(cpu);
            cpu.registers.pc = address;
            if constexpr (FlagTraits<Cond>::variable) return 16;
        }
        if constexpr (FlagTraits<Cond>::variable) return 12;
    }

    static void print(std::ostream &out, const Cpu &cpu) {
        out << "JP " << Cond::to_string();
        Loc::print_op(out, cpu);
    }
};

template<typename Cond, typename Loc>
struct JR : Operation<1+Loc::size>
{
    using result_type = typename FlagTraits<Cond>::result_type;

    static result_type execute(Cpu &cpu) {
        if (Cond::passes(cpu))
        {
            cpu.registers.pc += Loc::get(cpu);

            if constexpr (FlagTraits<Cond>::variable) return 12;
        }
        if constexpr (FlagTraits<Cond>::variable) return 8;
    }

    static void print(std::ostream &out, const Cpu &cpu) {
        out << "JR " << Cond::to_string();
        Loc::print_op(out, cpu);
    }
};

template<typename Loc>
struct PUSH : Operation<1+Loc::size>
{
    using result_type = void;

    static_assert (sizeof(typename Loc::data_type) == 2);

    static void execute(Cpu &cpu) {
        cpu.registers.sp -= 2;
        auto value = Loc::get(cpu);
        cpu.bus.write16(cpu.registers.sp, value);
    }

    static void print(std::ostream &out, const Cpu &cpu) {
        out << "PUSH ";
        Loc::print_op(out, cpu);
    }
};

template<typename Loc>
struct POP : Operation<1+Loc::size>
{
    using result_type = void;

    static_assert (sizeof(typename Loc::data_type) == 2);

    static void execute(Cpu &cpu) {
        auto value = cpu.bus.read16(cpu.registers.sp);
        cpu.registers.sp += 2;

        if constexpr (std::is_same_v<Loc, AF>)
        {
            value &= 0xFFF0;
        }

        Loc::put(cpu, value);
    }

    static void print(std::ostream &out, const Cpu &cpu) {
        out << "POP ";
        Loc::print_op(out, cpu);
    }
};

template<typename Cond, typename Loc>
struct CALL : Operation<1+Loc::size>
{
    using result_type = typename FlagTraits<Cond>::result_type;

    static result_type execute(Cpu &cpu) {
        if (Cond::passes(cpu))
        {
            PUSH<PC>::execute(cpu);
            JP<Always,Loc>::execute(cpu);
            if constexpr (FlagTraits<Cond>::variable) return 24;
        }
        if constexpr (FlagTraits<Cond>::variable) return 12;
    }

    static void print(std::ostream &out, const Cpu &cpu) {
        out << "CALL " << Cond::to_string();
        Loc::print_op(out, cpu);
    }
};

template<typename Cond=Always>
struct RET : Operation<1>
{
    using result_type = typename FlagTraits<Cond>::result_type;

    static result_type execute(Cpu &cpu) {
        if (Cond::passes(cpu))
        {
            POP<PC>::execute(cpu);
            if constexpr (FlagTraits<Cond>::variable) return 20;
        }
        if constexpr (FlagTraits<Cond>::variable) return 8;
    }

    static void print(std::ostream &out, const Cpu &cpu) {
        out << "RET " << Cond::to_string();
    }
};

template<std::uint16_t Addr>
struct RST : Operation<1>
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        PUSH<PC>::execute(cpu);
        cpu.registers.pc = Addr;
    }

    static void print(std::ostream &out, const Cpu &cpu) {
        out << "RST " << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << Addr;
    }
};

/// Returns and enables interrupgs
struct RETI : Operation<1>
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        EI::execute(cpu);
        RET<>::execute(cpu);
    }

    static void print(std::ostream &out, const Cpu &cpu) { out << "RETI"; }
};

template<typename Val>
struct AND : Operation<1+Val::size>
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        auto value = Val::get(cpu);

        cpu.registers.a &= value;

        cpu.registers.f = 0;
        cpu.registers.flags.z = cpu.registers.a == 0;
        cpu.registers.flags.h = true;
    }

    static void print(std::ostream &out, const Cpu &cpu) {
        out << "AND ";
        Val::print_op(out, cpu);
    }
};

template<typename Val>
struct OR : Operation<1+Val::size>
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        auto value = Val::get(cpu);

        cpu.registers.a |= value;

        cpu.registers.f = 0;
        cpu.registers.flags.z = cpu.registers.a == 0;
    }

    static void print(std::ostream &out, const Cpu &cpu) {
        out <<  "OR ";
        Val::print_op(out, cpu);
    }
};

template<typename Val>
struct XOR : Operation<1+Val::size>
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        auto value = Val::get(cpu);

        cpu.registers.a ^= value;

        cpu.registers.f = 0;
        cpu.registers.flags.z = cpu.registers.a == 0;
    }

    static void print(std::ostream &out, const Cpu &cpu) {
        out << "XOR ";
        Val::print_op(out, cpu);
    }
};

struct CPL : Operation<1>
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        cpu.registers.a = ~cpu.registers.a;
        cpu.registers.flags.n = true;
        cpu.registers.flags.h = true;
    }

    static void print(std::ostream &out, const Cpu &cpu) { out << "CPL"; }
};


template<typename Val>
struct INC : Operation<1>
{
    using result_type = void;

    static void execute(Cpu &cpu)
    {
        typename Val::data_type new_value = Val::get(cpu) + 1;
        Val::put(cpu, new_value);
        if constexpr (sizeof(typename Val::data_type) == 1)
        {
            cpu.registers.flags.z = new_value == 0;
            cpu.registers.flags.n = false;
            cpu.registers.flags.h = (new_value & 0x0F) == 0;
        }
    }

    static void print(std::ostream &out, const Cpu &cpu) {
        out << "INC ";
        Val::print_op(out, cpu);
    }
};

template<typename Val>
struct DEC : Operation<1>
{
    using result_type = void;

    static void execute(Cpu &cpu)
    {
        typename Val::data_type new_value= Val::get(cpu) - 1;
        Val::put(cpu, new_value);
        if constexpr (sizeof(typename Val::data_type) == 1)
        {
            cpu.registers.flags.z = new_value == 0;
            cpu.registers.flags.n = true;
            cpu.registers.flags.h = (new_value & 0x0F) == 0x0F;
        }
    }

    static void print(std::ostream &out, const Cpu &cpu) {
        out << "DEC ";
        Val::print_op(out, cpu);
    }
};

template<typename Val>
struct CP : Operation<1+Val::size>
{
    using result_type = void;

    static void execute(Cpu &cpu)
    {
        auto value= Val::get(cpu);
        auto diff = int(cpu.registers.a) - value;

        cpu.registers.flags.z = (diff == 0);
        cpu.registers.flags.n = true;
        cpu.registers.flags.h = ((value & 0x0f) > (cpu.registers.a & 0x0f));
        cpu.registers.flags.c = (diff < 0);
    }

    static void print(std::ostream &out, const Cpu &cpu) {
        out << "CP ";
        Val::print_op(out, cpu);;
    }
};


template<typename Dst, typename Src>
struct ADD : Operation<1+Dst::size+Src::size>
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        auto dst_value = Dst::get(cpu);
        auto src_value = Src::get(cpu);

        static_assert (sizeof(dst_value) == sizeof(src_value)
                || (sizeof(typename Dst::data_type) == 2 && sizeof(typename Src::data_type) == 1));

        std::uint32_t result = dst_value + src_value;

        if constexpr (sizeof(typename Dst::data_type) == 2 && sizeof(typename Src::data_type) == 1) //ADD SP, r8
        {            
            cpu.registers.flags.z = false;
            cpu.registers.flags.n = false;
            cpu.registers.flags.h = (dst_value & 0xF)  + (src_value & 0xF)  > 0xF;
            cpu.registers.flags.c = (dst_value & 0xFF) + (src_value & 0xFF) > 0xFF;

            Dst::put(cpu, result & 0xFFFF);
            return;
        }

        if constexpr (sizeof(typename Src::data_type) == 1)
        {
            std::uint8_t result8 = (result & 0xFF);

            cpu.registers.flags.z = result8 == 0;
            cpu.registers.flags.n = false;
            cpu.registers.flags.h = (dst_value & 0xF)  + (src_value & 0xF) > 0xF;
            cpu.registers.flags.c = (dst_value & 0xFF) + (src_value & 0xFF) > 0xFF;

            Dst::put(cpu, result8);
            return;
        }

        if constexpr (sizeof(typename Src::data_type) == 2)
        {
            cpu.registers.flags.n = false;
            cpu.registers.flags.h = (dst_value & 0xFFF) + (src_value & 0xFFF) > 0xFFF;
            cpu.registers.flags.c = result > 0xFFFF;

            Dst::put(cpu, result & 0xFFFF);
            return;
        }
    }

    static void print(std::ostream &out, const Cpu &cpu) {
        out << "ADD ";
        Dst::print_op(out, cpu);
        out << ", ";
        Src::print_op(out, cpu);
    }
};


template<typename Src>
struct SUB : Operation<1+Src::size>
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        auto dst_value = A::get(cpu);
        auto src_value = Src::get(cpu);

        static_assert (sizeof(dst_value) == 1);
        static_assert (sizeof(src_value) == 1);

        std::uint16_t result = dst_value - src_value;

        cpu.registers.flags.z = result == 0;
        cpu.registers.flags.n = true;
        cpu.registers.flags.h = (dst_value & 0xF) - (src_value & 0xF) < 0;
        cpu.registers.flags.c = dst_value - src_value < 0;

        A::put(cpu, result & 0xFF);
    }

    static void print(std::ostream &out, const Cpu &cpu) {
        out << "SUB ";
        Src::print_op(out, cpu);
    }
};


//Add with carry
template<typename Src>
struct ADC_A : Operation<1+Src::size>
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        uint16_t dst_value = cpu.registers.a;
        uint16_t src_value = Src::get(cpu);

        uint16_t carry = cpu.registers.flags.c ? 1 : 0;

        uint16_t result = dst_value + src_value + carry;
        cpu.registers.a = result & 0xFF;

        cpu.registers.flags.z = cpu.registers.a == 0;
        cpu.registers.flags.n = false;
        cpu.registers.flags.h = (dst_value & 0xF) + (src_value & 0xF) + carry > 0xF;
        cpu.registers.flags.c =  result > 0xFF;
    }

    static void print(std::ostream &out, const Cpu &cpu) {
        out << "ADC A, ";
        Src::print_op(out, cpu);
    }
};

//Subtract with carry
template<typename Src>
struct SBC_A : Operation<1+Src::size>
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        uint16_t dst_value = cpu.registers.a;
        uint16_t src_value = Src::get(cpu);

        uint16_t carry = cpu.registers.flags.c ? 1 : 0;

        uint16_t result = dst_value - src_value - carry;
        cpu.registers.a = result & 0xFF;

        cpu.registers.flags.z = cpu.registers.a == 0;
        cpu.registers.flags.n = true;

        cpu.registers.flags.h = (dst_value & 0xF) - (src_value & 0xF) - (carry) < 0;
        cpu.registers.flags.c = dst_value - src_value - carry < 0;

    }

    static void print(std::ostream &out, const Cpu &cpu) {
        out << "SBC A, ";
        Src::print_op(out, cpu);
    }
};

/// BCD addition
struct DAA : Operation<1>
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        uint16_t result = cpu.registers.a;

        if (cpu.registers.flags.n) //Subtraction
        {
            if (cpu.registers.flags.h) {
                result = (result - 0x06)&0xFF;
            }
            if (cpu.registers.flags.c) {
                result -= 0x60;
            }
        }
        else //Addition
        {
            if (cpu.registers.flags.h || (result & 0xF) > 9) {
                result += 0x06;
            }
            if (cpu.registers.flags.c || result > 0x9F) {
                result += 0x60;
            }
        }

        cpu.registers.a = result&0xFF;
        cpu.registers.flags.z = cpu.registers.a == 0;
        cpu.registers.flags.h = false;
        cpu.registers.flags.c = cpu.registers.flags.c || result > 0xFF;
    }

    static void print(std::ostream &out, const Cpu &cpu) { out <<  "DAA"; }
};

template<typename Val>
struct SWAP : Operation<2>
{
    using result_type = void;

    static void execute(Cpu &cpu)
    {
        auto value= Val::get(cpu);

        value = ((value & 0xf) << 4) | ((value & 0xf0) >> 4);

        cpu.registers.f = 0;
        cpu.registers.flags.z = value == 0;

        Val::put(cpu, value);
    }

    static void print(std::ostream &out, const Cpu &cpu) {
        out << "SWAP " << Val::to_string(cpu);
    }
};

/// Rotate left
template<typename Val>
struct RLC : Operation<2>
{
    using result_type = void;

    static void execute(Cpu &cpu)
    {
        auto value= Val::get(cpu);
        auto bit_7 = (value >> 7) & 1;
        auto result = (value << 1) | bit_7 & 0xFF;
        Val::put(cpu, result);

        cpu.registers.f = 0;
        cpu.registers.flags.z = result == 0;
        cpu.registers.flags.c = bit_7 != 0;
    }

    static void print(std::ostream &out, const Cpu &cpu) {
        out << "RLC " << Val::to_string(cpu);
    }
};

/// Rotate left A
struct RLCA : Operation<1>
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        RLC<A>::execute(cpu);
        cpu.registers.flags.z = false;
    }

    static void print(std::ostream &out, const Cpu &cpu) { out << "RLCA"; }
};

/// Rotate right
template<typename Val>
struct RRC : Operation<2>
{
    using result_type = void;

    static void execute(Cpu &cpu)
    {
        auto value= Val::get(cpu);
        auto bit_0 = value & 1;
        auto result = (value >> 1) | (bit_0 << 7);
        Val::put(cpu, result);

        cpu.registers.f = 0;
        cpu.registers.flags.z = result == 0;
        cpu.registers.flags.c = bit_0 != 0;
    }

    static void print(std::ostream &out, const Cpu &cpu) {
        out <<  "RRC " << Val::to_string(cpu);
    }
};

/// Rotate right A
struct RRCA : Operation<1>
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        RRC<A>::execute(cpu);
        cpu.registers.flags.z = false;
    }

    static void print(std::ostream &out, const Cpu &cpu) { out << "RRCA"; }
};

/// Rotate left through carry
template<typename Val>
struct RL : Operation<2>
{
    using result_type = void;

    static void execute(Cpu &cpu)
    {
        auto value= Val::get(cpu);
        auto bit_7 = value & 0x80;
        std::uint8_t result = (value << 1) | (cpu.registers.flags.c ? 1 : 0);
        Val::put(cpu, result);

        cpu.registers.f = 0;
        cpu.registers.flags.z = result == 0;
        cpu.registers.flags.c = bit_7 != 0;
    }

    static void print(std::ostream &out, const Cpu &cpu) {
        out << "RL " << Val::to_string(cpu);
    }
};

/// Rotate left A through carry
struct RLA : Operation<1>
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        RL<A>::execute(cpu);
        cpu.registers.flags.z = false;
    }

    static void print(std::ostream &out, const Cpu &cpu) { out << "RLA"; }
};

/// Rotate right through carry
template<typename Val>
struct RR : Operation<2>
{
    using result_type = void;

    static void execute(Cpu &cpu)
    {
        auto value= Val::get(cpu);
        auto bit_0 = value & 0x1;
        std::uint8_t result= (value >> 1) | (cpu.registers.flags.c ? 0x80 : 0);
        Val::put(cpu, result);

        cpu.registers.f = 0;
        cpu.registers.flags.z = result == 0;
        cpu.registers.flags.c = bit_0 != 0;
    }

    static void print(std::ostream &out, const Cpu &cpu) {
        out << "RR " << Val::to_string(cpu);
    }
};

/// Rotate right A through carry
struct RRA : Operation<1>
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        RR<A>::execute(cpu);
        cpu.registers.flags.z = false;
    }

    static void print(std::ostream &out, const Cpu &cpu) { out << "RRA"; }
};

template<typename Val>
struct SLA : Operation<2>
{
    using result_type = void;

    static void execute(Cpu &cpu)
    {
        auto value= Val::get(cpu);

        int8_t result = value << 1;
        Val::put(cpu, result);

        cpu.registers.f = 0;
        cpu.registers.flags.z = result == 0;
        cpu.registers.flags.c = value & 0x80;
    }

    static void print(std::ostream &out, const Cpu &cpu) {
        out << "SLA " << Val::to_string(cpu);
    }
};

template<typename Val>
struct SRA : Operation<2>
{
    using result_type = void;

    static void execute(Cpu &cpu)
    {
        std::int8_t value = Val::get(cpu);

        std::int8_t result = value >> 1;
        Val::put(cpu, result);

        cpu.registers.f = 0;
        cpu.registers.flags.z = result == 0;
        cpu.registers.flags.c = value & 1;
    }

    static void print(std::ostream &out, const Cpu &cpu) {
        out << "SRA " << Val::to_string(cpu);
    }
};

template<typename Val>
struct SRL : Operation<2>
{
    using result_type = void;

    static void execute(Cpu &cpu)
    {
        std::uint8_t value= Val::get(cpu);

        std::uint8_t result = value >> 1;
        Val::put(cpu, result);

        cpu.registers.f = 0;
        cpu.registers.flags.z = result == 0;
        cpu.registers.flags.c = value & 1;
    }

    static void print(std::ostream &out, const Cpu &cpu) {
        out << "SRL " << Val::to_string(cpu);
    }
};


template<std::uint8_t Bit, typename Val>
struct BIT : Operation<2>
{
    using result_type = void;

    static void execute(Cpu &cpu)
    {
        cpu.registers.flags.z = ! (Val::get(cpu) & (1 << Bit));
        cpu.registers.flags.n = false;
        cpu.registers.flags.h = true;
    }

    static void print(std::ostream &out, const Cpu &cpu) {
        out <<  "BIT " << std::to_string(Bit) << ", " << Val::to_string(cpu);
    }
};

template<std::uint8_t Bit, typename Val>
struct RES : Operation<2>
{
    using result_type = void;

    static void execute(Cpu &cpu)
    {
        auto val = Val::get(cpu);
        Val::put(cpu, val & ~(1 << Bit));
    }

    static void print(std::ostream &out, const Cpu &cpu) {
        out << "RES " << std::to_string(Bit) << ", " << Val::to_string(cpu);
    }
};

template<std::uint8_t Bit, typename Val>
struct SET : Operation<2>
{
    using result_type = void;

    static void execute(Cpu &cpu)
    {
        auto val = Val::get(cpu);
        Val::put(cpu, val | (1 << Bit));
    }

    static void print(std::ostream &out, const Cpu &cpu) {
        out << "SET " + std::to_string(Bit) << ", " << Val::to_string(cpu);
    }
};
