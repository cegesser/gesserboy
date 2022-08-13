#pragma once

#include "cpu.h"
#include <array>
#include <iomanip>
#include <sstream>

template<typename T, T CpuRegisters::*Ptr, char MN1, char MN2=0>
struct Register {
    using data_type = T;
    static constexpr std::uint8_t size = 0;

    static data_type read_data(const Cpu &cpu) {
        return cpu.registers.*Ptr;
    }

    static void write_data(Cpu &cpu, data_type value) {
        cpu.registers.*Ptr = value;
    }

    static std::string to_string(const Cpu &) { char str[3] = { MN1, MN2, 0 }; return str; }
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

    static data_type read_data(const Cpu &cpu)
    {
        auto lsb = cpu.arg1;
        auto msb = cpu.arg2;
        return lsb | msb << 8;
    }

    static std::string to_string(const Cpu &cpu)
    {
        char mnemonic[6] = {};
        std::uint16_t param = cpu.arg1 | cpu.arg2 << 8;
        sprintf_s(mnemonic, "%04X", param);

        return mnemonic;
    }
};

template<typename T>
struct Immediate8
{
    using data_type = T;
    static constexpr std::uint8_t size = sizeof (data_type);

    static data_type read_data(const Cpu &cpu)
    {
        return cpu.arg1;
    }

    static std::string to_string(const Cpu &cpu)
    {
        std::ostringstream out;
        out << std::hex << std::setw(2) << std::uppercase << std::setfill('0') << int(cpu.arg1);
        return out.str();
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

    static data_type read_data(Cpu &cpu)
    {
        auto r8_value = r8::read_data(cpu);
        cpu.registers.f = 0;
        cpu.registers.flags.h = (cpu.registers.sp & 0xF) + (r8_value & 0xF) >= 0x10;
        cpu.registers.flags.c = (cpu.registers.sp & 0xFF) + (r8_value & 0xFF) >= 0x100;
        return cpu.registers.sp + r8_value;
    }

    static std::string to_string(const Cpu &cpu)
    {
        return "SP+"+r8::to_string(cpu);
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

    static data_type read_data(Cpu &cpu)
    {
        std::uint16_t address = Loc::read_data(cpu);
        if constexpr (sizeof(Loc::data_type) == 1)
        {
            address += 0xFF00;
        }
        if constexpr (Op == '-')
        {
            Loc::write_data(cpu, address-1);
        }
        if constexpr (Op == '+')
        {
            Loc::write_data(cpu, address+1);
        }
        return cpu.bus.read(address);
    }

    static void write_data(Cpu &cpu, data_type value)
    {
        std::uint16_t address = Loc::read_data(cpu);
        if constexpr (sizeof(Loc::data_type) == 1)
        {
            address += 0xFF00;
        }
        if constexpr (Op == '-')
        {
            Loc::write_data(cpu, address-1);
        }
        if constexpr (Op == '+')
        {
            Loc::write_data(cpu, address+1);
        }
        cpu.bus.write(address, value);
    }

    template<typename Data>
    static std::string to_string(const Data &data)
    {
        if constexpr (sizeof(Loc::data_type) == 1)
        {
            return "(FF00+" + Loc::to_string(data) + ")";
        }
        if constexpr (Op == '-')
        {
            return "(" + Loc::to_string(data) + "-)";
        }
        if constexpr (Op == '+')
        {
            return "(" + Loc::to_string(data) + "+)";
        }
        return "(" + Loc::to_string(data) + ")";
    }
};

template<typename Loc>
struct At16
{
    using data_type = std::uint16_t;
    static constexpr std::uint8_t size = Loc::size;

    static_assert (sizeof(typename Loc::data_type) == 2);

    static data_type read_data(Cpu &cpu)
    {
        std::uint16_t address = Loc::read_data(cpu);
        return cpu.bus.read16(address);
    }

    static void write_data(Cpu &cpu, data_type value)
    {
        std::uint16_t address = Loc::read_data(cpu);
        cpu.bus.write16(address, value);
    }

    template<typename Data>
    static std::string to_string(const Data &data)
    {
        return "(" + Loc::to_string(data) + ")";
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
    static std::string mnemonic(const Cpu &) { return "INVALID"; }
};

struct NOP : Operation<1> {
    using result_type = void;

    static void execute(Cpu &) { }
    static std::string mnemonic(const Cpu &) { return "NOP"; }
};

struct HALT : Operation<1> {
    using result_type = void;

    static void execute(Cpu &cpu) { cpu.halted = true; }
    static std::string mnemonic(const Cpu &) { return "HALT"; }
};

struct STOP : Operation<2> {
    using result_type = void;

    static void execute(Cpu &) { /*TODO*/ }
    static std::string mnemonic(const Cpu &) { return "STOP"; }
};

/// Disables the master interrupt flag
struct DI : Operation<1> {
    using result_type = void;

    static void execute(Cpu &cpu) {
        cpu.inerrupts_master_enable_flag = false;
    }
    static std::string mnemonic(const Cpu &) {  return "DI"; }
};

/// Enables the master interrupt flag
struct EI : Operation<1> {
    using result_type = void;

    static void execute(Cpu &cpu) {
        cpu.inerrupts_master_enable_flag = true;
    }
    static std::string mnemonic(const Cpu &) {  return "EI"; }
};

/// Sets the carry flag
struct SCF : Operation<1> {
    using result_type = void;

    static void execute(Cpu &cpu) {
        cpu.registers.flags.n = false;
        cpu.registers.flags.h = false;
        cpu.registers.flags.c = true;
    }
    static std::string mnemonic(const Cpu &) {  return "SCF"; }
};

/// Flips the carry flag
struct CCF : Operation<1> {
    using result_type = void;

    static void execute(Cpu &cpu) {
        cpu.registers.flags.n = false;
        cpu.registers.flags.h = false;
        cpu.registers.flags.c = ! cpu.registers.flags.c;
    }
    static std::string mnemonic(const Cpu &) {  return "CCF"; }
};

struct PREFIX : Operation<2> {
    using result_type = std::size_t;

    static result_type execute(Cpu &) { return 0; }
    static std::string mnemonic(const Cpu &) {  return "PREFIX"; }
};

template<typename Dst, typename Src>
struct LD : Operation<1+Dst::size+Src::size>
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        auto value = Src::read_data(cpu);
        Dst::write_data(cpu, value);
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "LD " + Dst::to_string(cpu) + ", " + Src::to_string(cpu);
    }
};

template<typename Cond, typename Loc>
struct JP : Operation<1+Loc::size>
{
    using result_type = typename FlagTraits<Cond>::result_type;

    static result_type execute(Cpu &cpu) {
        if (Cond::passes(cpu))
        {
            auto address = Loc::read_data(cpu);
            cpu.registers.pc = address;
            if constexpr (FlagTraits<Cond>::variable) return 16;
        }
        if constexpr (FlagTraits<Cond>::variable) return 12;
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "JP " + Cond::to_string() + Loc::to_string(cpu);
    }
};

template<typename Cond, typename Loc>
struct JR : Operation<1+Loc::size>
{
    using result_type = typename FlagTraits<Cond>::result_type;

    static result_type execute(Cpu &cpu) {
        if (Cond::passes(cpu))
        {
            cpu.registers.pc += Loc::read_data(cpu);

            if constexpr (FlagTraits<Cond>::variable) return 12;
        }
        if constexpr (FlagTraits<Cond>::variable) return 8;
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "JR " + Cond::to_string() + Loc::to_string(cpu);
    }
};

template<typename Loc>
struct PUSH : Operation<1+Loc::size>
{
    using result_type = void;

    static_assert (sizeof(typename Loc::data_type) == 2);

    static void execute(Cpu &cpu) {
        cpu.registers.sp -= 2;
        auto value = Loc::read_data(cpu);
        cpu.bus.write16(cpu.registers.sp, value);
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "PUSH " + Loc::to_string(cpu);
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

        Loc::write_data(cpu, value);
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "POP " + Loc::to_string(cpu);
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

    static std::string mnemonic(const Cpu &cpu) {
        return "CALL " + Cond::to_string() + Loc::to_string(cpu);
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

    static std::string mnemonic(const Cpu &cpu) {
        return "RET " + Cond::to_string();
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

    static std::string mnemonic(const Cpu &cpu) {
        std::ostringstream out;
        out << "RST " << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << Addr;
        return out.str();
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

    static std::string mnemonic(const Cpu &cpu) {
        return "RETI";
    }
};

template<typename Val>
struct AND : Operation<1+Val::size>
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        auto value = Val::read_data(cpu);

        cpu.registers.a &= value;

        cpu.registers.f = 0;
        cpu.registers.flags.z = cpu.registers.a == 0;
        cpu.registers.flags.h = true;
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "AND " + Val::to_string(cpu);
    }
};

template<typename Val>
struct OR : Operation<1+Val::size>
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        auto value = Val::read_data(cpu);

        cpu.registers.a |= value;

        cpu.registers.f = 0;
        cpu.registers.flags.z = cpu.registers.a == 0;
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "OR " + Val::to_string(cpu);
    }
};

template<typename Val>
struct XOR : Operation<1+Val::size>
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        auto value = Val::read_data(cpu);

        cpu.registers.a ^= value;

        cpu.registers.f = 0;
        cpu.registers.flags.z = cpu.registers.a == 0;
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "XOR " + Val::to_string(cpu);
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

    static std::string mnemonic(const Cpu &cpu) {
        return "CPL";
    }
};


template<typename Val>
struct INC : Operation<1>
{
    using result_type = void;

    static void execute(Cpu &cpu)
    {
        Val::data_type new_value = Val::read_data(cpu) + 1;
        Val::write_data(cpu, new_value);
        if constexpr (sizeof(Val::data_type) == 1)
        {
            cpu.registers.flags.z = new_value == 0;
            cpu.registers.flags.n = false;
            cpu.registers.flags.h = (new_value & 0x0F) == 0;
        }
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "INC " + Val::to_string(cpu);
    }
};

template<typename Val>
struct DEC : Operation<1>
{
    using result_type = void;

    static void execute(Cpu &cpu)
    {
        Val::data_type new_value= Val::read_data(cpu) - 1;
        Val::write_data(cpu, new_value);
        if constexpr (sizeof(Val::data_type) == 1)
        {
            cpu.registers.flags.z = new_value == 0;
            cpu.registers.flags.n = true;
            cpu.registers.flags.h = (new_value & 0x0F) == 0x0F;
        }
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "DEC " + Val::to_string(cpu);
    }
};

template<typename Val>
struct CP : Operation<1+Val::size>
{
    using result_type = void;

    static void execute(Cpu &cpu)
    {
        auto value= Val::read_data(cpu);
        auto diff = int(cpu.registers.a) - value;

        cpu.registers.flags.z = (diff == 0);
        cpu.registers.flags.n = true;
        cpu.registers.flags.h = ((value & 0x0f) > (cpu.registers.a & 0x0f));
        cpu.registers.flags.c = (diff < 0);
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "CP " + Val::to_string(cpu);
    }
};


template<typename Dst, typename Src>
struct ADD : Operation<1+Dst::size+Src::size>
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        auto dst_value = Dst::read_data(cpu);
        auto src_value = Src::read_data(cpu);

        static_assert (sizeof(dst_value) == sizeof(src_value)
                || (sizeof(Dst::data_type) == 2 && sizeof(Src::data_type) == 1));

        std::uint32_t result = dst_value + src_value;

        if constexpr (sizeof(Dst::data_type) == 2 && sizeof(Src::data_type) == 1) //ADD SP, r8
        {            
            cpu.registers.flags.z = false;
            cpu.registers.flags.n = false;
            cpu.registers.flags.h = (dst_value & 0xF)  + (src_value & 0xF)  > 0xF;
            cpu.registers.flags.c = (dst_value & 0xFF) + (src_value & 0xFF) > 0xFF;

            Dst::write_data(cpu, result & 0xFFFF);
            return;
        }

        if constexpr (sizeof(Src::data_type) == 1)
        {
            std::uint8_t result8 = (result & 0xFF);

            cpu.registers.flags.z = result8 == 0;
            cpu.registers.flags.n = false;
            cpu.registers.flags.h = (dst_value & 0xF)  + (src_value & 0xF) > 0xF;
            cpu.registers.flags.c = (dst_value & 0xFF) + (src_value & 0xFF) > 0xFF;

            Dst::write_data(cpu, result8);
            return;
        }

        if constexpr (sizeof(Src::data_type) == 2)
        {
            cpu.registers.flags.n = false;
            cpu.registers.flags.h = (dst_value & 0xFFF) + (src_value & 0xFFF) > 0xFFF;
            cpu.registers.flags.c = result > 0xFFFF;

            Dst::write_data(cpu, result & 0xFFFF);
            return;
        }
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "ADD " + Dst::to_string(cpu) + ", " + Src::to_string(cpu);
    }
};


template<typename Src>
struct SUB : Operation<1+Src::size>
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        auto dst_value = A::read_data(cpu);
        auto src_value = Src::read_data(cpu);

        static_assert (sizeof(dst_value) == 1);
        static_assert (sizeof(src_value) == 1);

        std::uint16_t result = dst_value - src_value;

        cpu.registers.flags.z = result == 0;
        cpu.registers.flags.n = true;
        cpu.registers.flags.h = (dst_value & 0xF) - (src_value & 0xF) < 0;
        cpu.registers.flags.c = dst_value - src_value < 0;

        A::write_data(cpu, result & 0xFF);
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "SUB " + Src::to_string(cpu);
    }
};


//Add with carry
template<typename Src>
struct ADC_A : Operation<1+Src::size>
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        uint16_t dst_value = cpu.registers.a;
        uint16_t src_value = Src::read_data(cpu);

        uint16_t carry = cpu.registers.flags.c ? 1 : 0;

        uint16_t result = dst_value + src_value + carry;
        cpu.registers.a = result & 0xFF;

        cpu.registers.flags.z = cpu.registers.a == 0;
        cpu.registers.flags.n = false;
        cpu.registers.flags.h = (dst_value & 0xF) + (src_value & 0xF) + carry > 0xF;
        cpu.registers.flags.c =  result > 0xFF;
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "ADC A, " + Src::to_string(cpu);
    }
};

//Subtract with carry
template<typename Src>
struct SBC_A : Operation<1+Src::size>
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        uint16_t dst_value = cpu.registers.a;
        uint16_t src_value = Src::read_data(cpu);

        uint16_t carry = cpu.registers.flags.c ? 1 : 0;

        uint16_t result = dst_value - src_value - carry;
        cpu.registers.a = result & 0xFF;

        cpu.registers.flags.z = cpu.registers.a == 0;
        cpu.registers.flags.n = true;

        cpu.registers.flags.h = (dst_value & 0xF) - (src_value & 0xF) - (carry) < 0;
        cpu.registers.flags.c = dst_value - src_value - carry < 0;

    }

    static std::string mnemonic(const Cpu &cpu) {
        return "SBC A, " + Src::to_string(cpu);
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

    static std::string mnemonic(const Cpu &cpu) {
        return "DAA";
    }
};

template<typename Val>
struct SWAP : Operation<2>
{
    using result_type = void;

    static void execute(Cpu &cpu)
    {
        auto value= Val::read_data(cpu);

        value = ((value & 0xf) << 4) | ((value & 0xf0) >> 4);

        cpu.registers.f = 0;
        cpu.registers.flags.z = value == 0;

        Val::write_data(cpu, value);
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "SWAP " + Val::to_string(cpu);
    }
};

/// Rotate left
template<typename Val>
struct RLC : Operation<2>
{
    using result_type = void;

    static void execute(Cpu &cpu)
    {
        auto value= Val::read_data(cpu);
        auto bit_7 = (value >> 7) & 1;
        auto result = (value << 1) | bit_7 & 0xFF;
        Val::write_data(cpu, result);

        cpu.registers.f = 0;
        cpu.registers.flags.z = result == 0;
        cpu.registers.flags.c = bit_7 != 0;
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "RLC " + Val::to_string(cpu);
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

    static std::string mnemonic(const Cpu &cpu) {
        return "RLCA";
    }
};

/// Rotate right
template<typename Val>
struct RRC : Operation<2>
{
    using result_type = void;

    static void execute(Cpu &cpu)
    {
        auto value= Val::read_data(cpu);
        auto bit_0 = value & 1;
        auto result = (value >> 1) | (bit_0 << 7);
        Val::write_data(cpu, result);

        cpu.registers.f = 0;
        cpu.registers.flags.z = result == 0;
        cpu.registers.flags.c = bit_0 != 0;
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "RRC " + Val::to_string(cpu);
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

    static std::string mnemonic(const Cpu &cpu) {
        return "RRCA";
    }
};

/// Rotate left through carry
template<typename Val>
struct RL : Operation<2>
{
    using result_type = void;

    static void execute(Cpu &cpu)
    {
        auto value= Val::read_data(cpu);
        auto bit_7 = value & 0x80;
        std::uint8_t result = (value << 1) | (cpu.registers.flags.c ? 1 : 0);
        Val::write_data(cpu, result);

        cpu.registers.f = 0;
        cpu.registers.flags.z = result == 0;
        cpu.registers.flags.c = bit_7 != 0;
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "RL " + Val::to_string(cpu);
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

    static std::string mnemonic(const Cpu &cpu) {
        return "RLA";
    }
};

/// Rotate right through carry
template<typename Val>
struct RR : Operation<2>
{
    using result_type = void;

    static void execute(Cpu &cpu)
    {
        auto value= Val::read_data(cpu);
        auto bit_0 = value & 0x1;
        std::uint8_t result= (value >> 1) | (cpu.registers.flags.c ? 0x80 : 0);
        Val::write_data(cpu, result);

        cpu.registers.f = 0;
        cpu.registers.flags.z = result == 0;
        cpu.registers.flags.c = bit_0 != 0;
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "RR " + Val::to_string(cpu);
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

    static std::string mnemonic(const Cpu &cpu) {
        return "RRA";
    }
};

template<typename Val>
struct SLA : Operation<2>
{
    using result_type = void;

    static void execute(Cpu &cpu)
    {
        auto value= Val::read_data(cpu);

        int8_t result = value << 1;
        Val::write_data(cpu, result);

        cpu.registers.f = 0;
        cpu.registers.flags.z = result == 0;
        cpu.registers.flags.c = value & 0x80;
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "SLA " + Val::to_string(cpu);
    }
};

template<typename Val>
struct SRA : Operation<2>
{
    using result_type = void;

    static void execute(Cpu &cpu)
    {
        std::int8_t value = Val::read_data(cpu);

        std::int8_t result = value >> 1;
        Val::write_data(cpu, result);

        cpu.registers.f = 0;
        cpu.registers.flags.z = result == 0;
        cpu.registers.flags.c = value & 1;
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "SRA " + Val::to_string(cpu);
    }
};

template<typename Val>
struct SRL : Operation<2>
{
    using result_type = void;

    static void execute(Cpu &cpu)
    {
        std::uint8_t value= Val::read_data(cpu);

        std::uint8_t result = value >> 1;
        Val::write_data(cpu, result);

        cpu.registers.f = 0;
        cpu.registers.flags.z = result == 0;
        cpu.registers.flags.c = value & 1;
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "SRL " + Val::to_string(cpu);
    }
};


template<std::uint8_t Bit, typename Val>
struct BIT : Operation<2>
{
    using result_type = void;

    static void execute(Cpu &cpu)
    {
        cpu.registers.flags.z = ! (Val::read_data(cpu) & (1 << Bit));
        cpu.registers.flags.n = false;
        cpu.registers.flags.h = true;
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "BIT " + std::to_string(Bit)+ ", " + Val::to_string(cpu);
    }
};

template<std::uint8_t Bit, typename Val>
struct RES : Operation<2>
{
    using result_type = void;

    static void execute(Cpu &cpu)
    {
        auto val = Val::read_data(cpu);
        Val::write_data(cpu, val & ~(1 << Bit));
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "RES " + std::to_string(Bit)+ ", " + Val::to_string(cpu);
    }
};

template<std::uint8_t Bit, typename Val>
struct SET : Operation<2>
{
    using result_type = void;

    static void execute(Cpu &cpu)
    {
        auto val = Val::read_data(cpu);
        Val::write_data(cpu, val | (1 << Bit));
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "SET " + std::to_string(Bit)+ ", " + Val::to_string(cpu);
    }
};
