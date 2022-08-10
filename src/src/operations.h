#pragma once

#include "cpu.h"
#include <array>
#include <iomanip>
#include <sstream>

template<std::size_t Size>
struct Impl
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

struct NotImplemented : Impl<0> {
    using result_type = size_t;

    static result_type execute(Cpu &)
    {
        throw std::runtime_error("Not impl instruction");
    }
    static std::string mnemonic(const Cpu &) {  return "INVALID"; }
};

template<typename T, T CpuRegisters::*Ptr, char MN1, char MN2=0>
struct Reg {
    using data_type = T;
    static constexpr std::uint8_t size = 0;

    static data_type read_data(const Cpu &cpu) {
        //std::cout << "Reading " << std::hex << int(cpu.registers.*Ptr) << " from " << to_string(0) << std::endl;
        return cpu.registers.*Ptr;
    }

    static void write_data(Cpu &cpu, data_type value) {
        //std::cout << "Writing " << std::hex << (int)data << " to " << to_string(0) << std::endl;
        cpu.registers.*Ptr = value;
    }

    static std::string to_string(const Cpu &) { char str[3] = { MN1, MN2, 0 }; return str; }
};

struct A : Reg<std::uint8_t, &CpuRegisters::a, 'A'> {};
struct B : Reg<std::uint8_t, &CpuRegisters::b, 'B'> {};
struct C : Reg<std::uint8_t, &CpuRegisters::c, 'C'> {};
struct D : Reg<std::uint8_t, &CpuRegisters::d, 'D'> {};
struct E : Reg<std::uint8_t, &CpuRegisters::e, 'E'> {};
struct H : Reg<std::uint8_t, &CpuRegisters::h, 'H'> {};
struct L : Reg<std::uint8_t, &CpuRegisters::l, 'L'> {};

struct AF : Reg<std::uint16_t, &CpuRegisters::af, 'A', 'F'> {};
struct BC : Reg<std::uint16_t, &CpuRegisters::bc, 'B', 'C'> {};
struct DE : Reg<std::uint16_t, &CpuRegisters::de, 'D', 'E'> {};
struct HL : Reg<std::uint16_t, &CpuRegisters::hl, 'H', 'L'> {};
struct PC : Reg<std::uint16_t, &CpuRegisters::pc, 'P', 'C'> {};
struct SP : Reg<std::uint16_t, &CpuRegisters::sp, 'S', 'P'> {};

struct Im16
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

using d16 = Im16;
using a16 = Im16;

template<typename T>
struct Im8
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

struct a8 : Im8<std::uint8_t> {};
struct d8 : Im8<std::uint8_t> {};
struct r8 : Im8<std::int8_t> {};

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
//        if (cpu.registers.pc == 589)
//        {
//            std::cout << "PC = " << cpu.registers.pc << std::endl;
//        }
        std::uint16_t address = Loc::read_data(cpu);
        if constexpr (sizeof(Loc::data_type) == 1)
        {
            address += 0xFF00;
        }
        //std::cout << "Writing " << std::hex << (int)data << " at " << address << std::endl;

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
            return "(^" + Loc::to_string(data) + ")";
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

struct INVALID : Impl<1> {
    using result_type = void;

    static void execute(Cpu &) { throw std::runtime_error("Invalid instruction"); }
    static std::string mnemonic(const Cpu &) {  return "INVALID"; }
};

struct NOP : Impl<1> {
    using result_type = void;

    static void execute(Cpu &) { }
    static std::string mnemonic(const Cpu &) {  return "NOP"; }
};

struct HALT : Impl<1> {
    using result_type = void;

    static void execute(Cpu &cpu) { cpu.halted = true; }
    static std::string mnemonic(const Cpu &) {  return "HALT"; }
};


struct STOP : Impl<2> {
    using result_type = void;

    static void execute(Cpu &) { }
    static std::string mnemonic(const Cpu &) {  return "STOP"; }
};

struct PREFIX : Impl<2> {
    using result_type = std::size_t;

    static result_type execute(Cpu &) { return 0; }
    static std::string mnemonic(const Cpu &) {  return "PREFIX"; }
};

struct DI : Impl<1> {
    using result_type = void;

    static void execute(Cpu &cpu) {
        cpu.bus.interrupts_master_enable_flag = false;
    }
    static std::string mnemonic(const Cpu &) {  return "DI"; }
};

struct EI : Impl<1> {
    using result_type = void;

    static void execute(Cpu &cpu) {
        cpu.bus.interrupts_master_enable_flag = true;
    }
    static std::string mnemonic(const Cpu &) {  return "EI"; }
};

struct SCF : Impl<1> {
    using result_type = void;

    static void execute(Cpu &cpu) {
        cpu.registers.flags.n = false;
        cpu.registers.flags.h = false;
        cpu.registers.flags.c = true;
    }
    static std::string mnemonic(const Cpu &) {  return "SCF"; }
};

struct CCF : Impl<1> {
    using result_type = void;

    static void execute(Cpu &cpu) {
        cpu.registers.flags.n = false;
        cpu.registers.flags.h = false;
        cpu.registers.flags.c = (cpu.registers.flags.c ^ 1) != 0;
    }
    static std::string mnemonic(const Cpu &) {  return "CCF"; }
};

template<typename Dst, typename Src>
struct LD : Impl<1+Dst::size+Src::size>
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
struct JP : Impl<1+Loc::size>
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
struct JR : Impl<1+Loc::size>
{
    using result_type = typename FlagTraits<Cond>::result_type;

    static result_type execute(Cpu &cpu) {
        if (Cond::passes(cpu))
        {
            auto delta = Loc::read_data(cpu);
            cpu.registers.pc += delta;

            if constexpr (FlagTraits<Cond>::variable) return 12;
        }
        if constexpr (FlagTraits<Cond>::variable) return 8;
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "JR " + Cond::to_string() + Loc::to_string(cpu);
    }
};

template<typename Loc>
struct PUSH : Impl<1+Loc::size>
{
    using result_type = void;

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
struct POP : Impl<1+Loc::size>
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
struct CALL : Impl<1+Loc::size>
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
struct RET : Impl<1>
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
struct RST : Impl<1>
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

struct RETI : Impl<1>
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        cpu.bus.interrupts_master_enable_flag = true;
        RET<>::execute(cpu);
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "RETI";
    }
};

template<typename Reg>
struct AND : Impl<1+Reg::size>
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        auto value = Reg::read_data(cpu);

        cpu.registers.a &= value;// & 0xFF;
        cpu.registers.f = 0;
        cpu.registers.flags.z = cpu.registers.a == 0;
        cpu.registers.flags.h = true;
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "AND " + Reg::to_string(cpu);
    }
};

template<typename Reg>
struct OR : Impl<1+Reg::size>
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        auto value = Reg::read_data(cpu);

        cpu.registers.a |= value & 0xFF;
        cpu.registers.f = 0;
        cpu.registers.flags.z = cpu.registers.a == 0;
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "OR " + Reg::to_string(cpu);
    }
};

template<typename Reg>
struct XOR : Impl<1+Reg::size>
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        auto value = Reg::read_data(cpu);

        cpu.registers.a ^= value & 0xFF;
        cpu.registers.f = 0;
        cpu.registers.flags.z = cpu.registers.a == 0;
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "XOR " + Reg::to_string(cpu);
    }
};

struct CPL : Impl<1>
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

struct DAA : Impl<1>
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        /*
        int carry = (cpu.registers.flags.c ? 1 : 0) << 7;
        cpu.registers.f = 0;
        cpu.registers.flags.c = (cpu.registers.a & 0x01);
        cpu.registers.a = (cpu.registers.a >> 1) + carry;

        uint8_t u = 0;
        int fc = 0;

        if (cpu.registers.flags.h || (!cpu.registers.flags.n && (cpu.registers.a & 0xF) > 9)) {
            u = 6;
        }

        if (cpu.registers.flags.c || (!cpu.registers.flags.n && cpu.registers.a > 0x99)) {
            u |= 0x60;
            fc = 1;
        }

        cpu.registers.a += cpu.registers.flags.n ? -u : u;

        cpu.registers.flags.z = cpu.registers.a == 0;
        cpu.registers.flags.h = 0;
        cpu.registers.flags.c = fc;
        */
        unsigned short s = cpu.registers.a;

        if (cpu.registers.flags.n) {
            if(cpu.registers.flags.h) s = (s - 0x06)&0xFF;
            if(cpu.registers.flags.c) s -= 0x60;
        }
        else {
            if(cpu.registers.flags.h || (s & 0xF) > 9) s += 0x06;
            if(cpu.registers.flags.c || s > 0x9F) s += 0x60;
        }

        cpu.registers.a = s;
        cpu.registers.flags.h = false;

        if(cpu.registers.a) cpu.registers.flags.z = false;
        else cpu.registers.flags.z = true;

        if(s >= 0x100) cpu.registers.flags.c = true;
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "DAA";
    }
};

struct RLCA : Impl<1>
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        auto value = cpu.registers.a;
        int c_flag = (value >> 7) & 1;

        cpu.registers.f = 0;
        cpu.registers.flags.c = c_flag;

        cpu.registers.a = (value << 1) | c_flag;
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "RLCA";
    }
};

struct RRCA : Impl<1>
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        auto value = cpu.registers.a & 1;

        cpu.registers.f = 0;
        cpu.registers.flags.c = value;

        cpu.registers.a = (cpu.registers.a >> 1) | (value << 7);
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "RRCA";
    }
};

struct RLA : Impl<1>
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        std::uint8_t value = cpu.registers.a ;
        std::uint8_t carry = cpu.registers.flags.c ? 1 : 0;

        cpu.registers.a = (value << 1) | carry;
        cpu.registers.f = 0;
        cpu.registers.flags.c = (value >> 7) & 1;
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "RLA";
    }
};

struct RRA : Impl<1>
{
    using result_type = void;

    static void execute(Cpu &cpu) {

        uint8_t carry = (cpu.registers.flags.c ? 1 : 0);
        uint8_t new_c = cpu.registers.a & 1;

        cpu.registers.a >>= 1;
        cpu.registers.a |= (carry << 7);

        cpu.registers.f = 0;
        cpu.registers.flags.c = new_c;

//        int carry = (cpu.registers.flags.c ? 1 : 0) << 7;
//        cpu.registers.f = 0;
//        cpu.registers.flags.c = (cpu.registers.a & 0x01);
//        cpu.registers.a = (cpu.registers.a >> 1) + carry;
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "RRA";
    }
};

template<typename Op>
struct INC : Impl<1>
{
    using result_type = void;

    static void execute(Cpu &cpu)
    {
        auto old = Op::read_data(cpu);


        Op::data_type value = Op::read_data(cpu) + 1;
        Op::write_data(cpu, value);
        if constexpr (sizeof(Op::data_type) == 1)
        {
            cpu.registers.flags.z = value == 0;
            cpu.registers.flags.n = false;
            cpu.registers.flags.h = (value & 0x0F) == 0;
        }
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "INC " + Op::to_string(cpu);
    }
};

template<typename Op>
struct DEC : Impl<1>
{
    using result_type = void;

    static void execute(Cpu &cpu)
    {
        Op::data_type value= Op::read_data(cpu) - 1;
        Op::write_data(cpu, value);
        if constexpr (sizeof(Op::data_type) == 1)
        {
            cpu.registers.flags.z = value == 0;
            cpu.registers.flags.n = true;
            cpu.registers.flags.h = (value & 0x0F) == 0x0F;
        }
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "DEC " + Op::to_string(cpu);
    }
};

template<typename Val>
struct CP : Impl<1+Val::size>
{
    using result_type = void;

    static void execute(Cpu &cpu)
    {
        auto value= Val::read_data(cpu);

        int diff = int(cpu.registers.a) - value;

        cpu.registers.flags.z = (diff == 0);
        cpu.registers.flags.n = true;


        cpu.registers.flags.h = ((value & 0x0f) > (cpu.registers.a & 0x0f));

        cpu.registers.flags.c = (diff < 0);

//            cpu_set_flags(ctx, n == 0, 1,
//                ((int)ctx->regs.a & 0x0F) - ((int)ctx->fetched_data & 0x0F) < 0, n < 0);
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "CP " + Val::to_string(cpu);
    }
};


template<typename Dst, typename Src>
struct ADD : Impl<1+Dst::size+Src::size>
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        auto dst_value = Dst::read_data(cpu);
        auto src_value = Src::read_data(cpu);

        std::uint32_t result32 = dst_value + src_value;

        static_assert (sizeof(dst_value) == sizeof(src_value)
                || (sizeof(Dst::data_type) == 2 && sizeof(Src::data_type) == 1));

        if constexpr (sizeof(Dst::data_type) == 2 && sizeof(Src::data_type) == 1)
        {
            //ADD SP, r8
            //2  16
            //0 0 H C
            cpu.registers.flags.z = false;
            cpu.registers.flags.n = false;

            cpu.registers.flags.h =    (dst_value & 0xF)  +    (src_value & 0xF) >= 0x10;
            cpu.registers.flags.c = int(dst_value & 0xFF) + int(src_value & 0xFF) >= 0x100;

            Dst::write_data(cpu, result32 & 0xFFFF);
            return;
        }

        if constexpr (sizeof(Src::data_type) == 1)
        {
            //ADD<A,d8> ???

            std::uint8_t result = (result32 & 0xFF);

            cpu.registers.flags.z = result == 0;
            cpu.registers.flags.n = false;
            cpu.registers.flags.h =    (dst_value & 0xF) +     (src_value & 0xF) >= 0x10;
            cpu.registers.flags.c = int(dst_value & 0xFF) + int(src_value & 0xFF) >= 0x100;

            Dst::write_data(cpu, result);
            return;
        }

        if constexpr (sizeof(Src::data_type) == 2)
        {
            cpu.registers.flags.n = false;
            cpu.registers.flags.h = (dst_value & 0xFFF)
                                  + (src_value & 0xFFF) >= 0x1000;
            cpu.registers.flags.c = result32 >= 0x10000;

            Dst::write_data(cpu, result32 & 0xFFFF);
            return;
        }


    }

    static std::string mnemonic(const Cpu &cpu) {
        return "ADD " + Dst::to_string(cpu) + ", " + Src::to_string(cpu);
    }
};


template<typename Src>
struct ADC_A : Impl<1+Src::size>
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

template<typename Src>
struct SUB : Impl<1+Src::size>
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
        cpu.registers.flags.h = (int(dst_value) & 0xF) - (int(src_value) & 0xF) < 0;
        cpu.registers.flags.c = (int(dst_value))       - (int(src_value)) < 0;

        A::write_data(cpu, result & 0xFF);
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "SUB " + Src::to_string(cpu);
    }
};

template<typename Src>
struct SBC_A : Impl<1+Src::size>
{
    using result_type = void;

    static void execute(Cpu &cpu) {

        uint16_t src_value = Src::read_data(cpu);
        uint8_t val = src_value + (cpu.registers.flags.c ? 1 : 0);

        cpu.registers.flags.z = cpu.registers.a - val == 0;
        cpu.registers.flags.n = true;

        cpu.registers.flags.h = (int(cpu.registers.a) & 0xF)
                              - (int(src_value) & 0xF)
                              - (cpu.registers.flags.c ? 1 : 0) < 0;
        cpu.registers.flags.c = int(cpu.registers.a)
                              - int(src_value)
                              - (cpu.registers.flags.c ? 1 : 0) < 0;

        cpu.registers.a  = cpu.registers.a - val;

    }

    static std::string mnemonic(const Cpu &cpu) {
        return "SBC A, " + Src::to_string(cpu);
    }
};


template<typename Val>
struct SWAP : Impl<2>
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

template<typename Val>
struct RLC : Impl<2>
{
    using result_type = void;

    static void execute(Cpu &cpu)
    {
        bool setC = false;
        auto value= Val::read_data(cpu);
        std::uint8_t result = (value << 1) & 0xFF;

        if ((value & (1 << 7)) != 0) {
            result |= 1;
            setC = true;
        }

        cpu.registers.f = 0;
        cpu.registers.flags.z = result == 0;
        cpu.registers.flags.c = setC;

        Val::write_data(cpu, result);
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "RLC " + Val::to_string(cpu);
    }
};


template<typename Val>
struct RRC : Impl<2>
{
    using result_type = void;

    static void execute(Cpu &cpu)
    {
        auto value= Val::read_data(cpu);
        auto old = value;

        value >>= 1;
        value |= (old << 7);

        cpu.registers.f = 0;
        cpu.registers.flags.z = value == 0;
        cpu.registers.flags.c = old & 1;

        Val::write_data(cpu, value);
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "RRC " + Val::to_string(cpu);
    }
};

template<typename Val>
struct RL : Impl<2>
{
    using result_type = void;

    static void execute(Cpu &cpu)
    {
        auto value= Val::read_data(cpu);
        std::uint8_t result = value << 1;
        result |= cpu.registers.flags.c ? 1 : 0;
        Val::write_data(cpu, result);

        cpu.registers.f = 0;
        cpu.registers.flags.z = result == 0;
        cpu.registers.flags.c = (value & 0x80) != 0;
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "RL " + Val::to_string(cpu);
    }
};

template<typename Val>
struct RR : Impl<2>
{
    using result_type = void;

    static void execute(Cpu &cpu)
    {
        auto value= Val::read_data(cpu);
        std::uint8_t result = value >> 1;
        result |= cpu.registers.flags.c ? (1<<7) : 0;
        Val::write_data(cpu, result);

        cpu.registers.f = 0;
        cpu.registers.flags.z = result == 0;
        cpu.registers.flags.c = (value & 1) != 0;
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "RR " + Val::to_string(cpu);
    }
};

template<typename Val>
struct SLA : Impl<2>
{
    using result_type = void;

    static void execute(Cpu &cpu)
    {
        auto value= Val::read_data(cpu);

        auto old = value;
        value <<= 1;

        Val::write_data(cpu, value);

        cpu.registers.f = 0;
        cpu.registers.flags.z = value == 0;
        cpu.registers.flags.c = old & 0x80;
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "SLA " + Val::to_string(cpu);
    }
};

template<typename Val>
struct SRA : Impl<2>
{
    using result_type = void;

    static void execute(Cpu &cpu)
    {
        std::int8_t value = Val::read_data(cpu);

        int8_t result = value >> 1;
        Val::write_data(cpu, result);

        cpu.registers.f = 0;
        cpu.registers.flags.z = !result;
        cpu.registers.flags.c = value & 1;
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "SRA " + Val::to_string(cpu);
    }
};

template<typename Val>
struct SRL : Impl<2>
{
    using result_type = void;

    static void execute(Cpu &cpu)
    {
        auto value= Val::read_data(cpu);
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
struct BIT : Impl<2>
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
struct RES : Impl<2>
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
struct SET : Impl<2>
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
