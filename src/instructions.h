#pragma once

#include "cpu.h"
#include <array>
#include <iomanip>
#include <sstream>

template<typename Int8>
void inc_dec(Cpu &cpu, Int8 &v, int delta)
{
    std::uint8_t value { v };
    if (delta > 0)
    {
        cpu.registers.flags.h = (value & 0x0f) == 0x0f;
    }
    if (delta < 0)
    {
        cpu.registers.flags.h = ! (value & 0x0f) ;
    }

    value += delta;

    v = value;

    cpu.registers.flags.z = value == 0;

    cpu.registers.flags.n = delta < 0;
}

void op_add(Cpu &cpu, std::uint8_t &result, std::uint8_t value)
{
    std::uint32_t result32 = result + value;

    cpu.registers.flags.c = (result32 & 0xff00);

    result = (result32 & 0xff);

    cpu.registers.flags.n = false;
    cpu.registers.flags.z = result == 0;
    cpu.registers.flags.h = (((result & 0x0f) + (value & 0x0f)) > 0x0f);

}

void op_adc(Cpu &cpu, std::uint8_t &result, std::uint8_t value)
{
    std::uint32_t result32 = cpu.registers.a + value + (cpu.registers.flags.c ? 1 : 0);

    cpu.registers.flags.c = (result32 & 0xff00);

    cpu.registers.flags.n = false;
    cpu.registers.flags.z = result == 0;
    cpu.registers.flags.h = ((value & 0x0f) + (result & 0x0f)) > 0x0f;;

    result = (result & 0xff);
}

void op_add(Cpu &cpu, std::uint16_t &result, std::uint16_t value)
{
    std::uint32_t result32 = result + value;

    cpu.registers.flags.c = (result32 & 0xffff0000);

    result = (result32 & 0xffff);

    cpu.registers.flags.h = (((result & 0x0f) + (value & 0x0f)) > 0x0f);

    cpu.registers.flags.n = false;
}


template<typename Int8>
void op_sla(Cpu &cpu, Int8 &v)
{
    std::uint8_t value = v;
    cpu.registers.f = 0;
    cpu.registers.flags.c = (value & 0x80);
    v = std::uint8_t(value << 1);
    cpu.registers.flags.z = value == 0;
}

std::uint8_t op_swap(Cpu &cpu, std::uint8_t value) {
    value = ((value & 0xf) << 4) | ((value & 0xf0) >> 4);

    cpu.registers.f = 0;
    cpu.registers.flags.z = value == 0;

    return value;
}

void op_bit(Cpu &cpu, std::uint8_t bit, std::uint8_t value) {
    cpu.registers.flags.z = ! (value & bit);
    cpu.registers.flags.n = false;
    cpu.registers.flags.h = true;
}

template<typename Int8>
void op_res(std::uint8_t bit, Int8 &v) {
    const std::uint8_t value = v;
    v = std::uint8_t(value & ~(1 << bit));
}

template<typename Int8>
void op_set(std::uint8_t bit, Int8 &v) {
    const std::uint8_t value = v;
    v = std::uint8_t(value | bit);
}

std::size_t op_jump_rel(Cpu &cpu, bool cond, std::int8_t param) {
    if ( ! cond ) return 8;
    cpu.registers.pc += param; return 12;
};

void op_rotate_carry(Cpu &cpu, bool left)
{
    unsigned char carry = left
            ? (cpu.registers.a & 0x80) >> 7
            :  cpu.registers.a & 0x01;

    cpu.registers.f = 0;
    cpu.registers.flags.c = carry != 0;

    if (left)
    {
        cpu.registers.a <<= 1;
        if (carry) cpu.registers.a += carry;
    }
    else
    {
        cpu.registers.a >>= 1;
        if (carry) cpu.registers.a |= 0x80;
    }
}

void op_push(Cpu &cpu, std::uint16_t value)
{
    cpu.registers.sp -= 2;
    cpu.bus[cpu.registers.sp] = value;
}

std::uint16_t op_pop(Cpu &cpu)
{
    auto result = cpu.bus[cpu.registers.sp];
    cpu.registers.sp += 2;
    return result;
}

template<std::uint16_t OPCODE, std::int8_t SIZE, std::uint8_t TICKS, const char *MNEMONIC>
struct InstructionBase
{
    static constexpr std::uint16_t opcode = OPCODE;
    static constexpr std::int16_t size = SIZE;
    static constexpr std::uint16_t usize = SIZE > 0 ? SIZE : -SIZE;
    static constexpr bool signed_arg = SIZE < 0;
    static constexpr std::size_t ticks = TICKS;
    static constexpr const char *mnemonic = MNEMONIC;

    static constexpr bool new_style = false;
};

template<std::uint16_t OPCODE>
struct Instruction : InstructionBase<OPCODE, 0, 0, nullptr>{};


template<typename Inst, typename Data>
std::string inst_to_str(const Inst &inst, const Cpu &cpu, const Data &data)
{
    //std::cout << cpu.last_inst_str << std::endl;
    std::ostringstream out;

    if constexpr (inst.usize == 0)
    {
        return "";
    }
    out << "[" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << int(data[0]);
    if constexpr (inst.usize > 1)
    {
        out << " " << std::setw(2) << std::setfill('0') << int(data[1]);
    }
    else
    {
        out << "   ";
    }
    if constexpr (inst.usize > 2)
    {
        out << " " << std::setw(2) << std::setfill('0') << int(data[2]);
    }
    else
    {
        out << "   ";
    }

    out << "] ";

    if constexpr (inst.usize > 0)
    {
        std::string raw_mnemonic = InstructionTraits<Inst>::mnemonic(cpu);
        if (auto pos = raw_mnemonic.find('8'); pos != std::string::npos)
        {
            //raw_mnemonic.replace(pos-1, 2, "%02X");
        }
        if (auto pos = raw_mnemonic.find("16"); pos != std::string::npos)
        {
            //raw_mnemonic.replace(pos-1, 3, "%04X");
        }

        char mnemonic[20] = {0};
        if constexpr (inst.usize == 1)
        {
            sprintf_s(mnemonic, "%s", raw_mnemonic.c_str());
        }
        if constexpr (inst.usize == 2)
        {
            sprintf_s(mnemonic, raw_mnemonic.c_str(), data[1]);
        }
        if constexpr (inst.usize == 3)
        {
            std::uint16_t param = data[1] | data[2] << 8;
            sprintf_s(mnemonic, raw_mnemonic.c_str(), param);
        }

        out << mnemonic;
    }

    return out.str();
}


#define INST(OPCODE, MNEMONIC, SIZE, TICKS, ...)                                           \
static char INST_ ## OPCODE[] = MNEMONIC;                                                   \
template<> struct Instruction<OPCODE> : InstructionBase<OPCODE, SIZE, TICKS, INST_ ## OPCODE> {\
     }; \
std::conditional_t<TICKS==0,std::size_t,void> call(Instruction<OPCODE>, __VA_ARGS__)

#define INST(OPCODE, MNEMONIC, SIZE, TICKS, ...) \
    std::conditional_t<TICKS==0,std::size_t,void> call ## OPCODE(__VA_ARGS__)

template<std::int8_t SIZE, std::uint8_t TICKS, typename Impl>
struct Inst {

    static constexpr std::int16_t size = SIZE;
    static constexpr std::uint16_t usize = size > 0 ? size : -size;
    static constexpr bool signed_arg = size < 0;
    static constexpr std::size_t ticks = TICKS;

    using impl_type = Impl;
    using result_type = typename Impl::result_type;

    static constexpr bool new_style = true;
};


template<typename T, T CpuRegisters::*Ptr, char MN1, char MN2=0>
struct Reg {
    using data_type = T;

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
        return cpu.bus[address];
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
        cpu.bus[address] = value;
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

    static data_type read_data(Cpu &cpu)
    {
        std::uint16_t address = Loc::read_data(cpu);
        return cpu.bus[address];
    }

    static void write_data(Cpu &cpu, data_type value)
    {
        std::uint16_t address = Loc::read_data(cpu);
        cpu.bus[address] = value;
    }

    template<typename Data>
    static std::string to_string(const Data &data)
    {
        return "(" + Loc::to_string(data) + ")";
    }
};

struct NOP{
    using result_type = void;

    static void execute(Cpu &) { }
    static std::string mnemonic(const Cpu &) {  return "NOP"; }
};

struct STOP{
    using result_type = void;

    static void execute(Cpu &) { }
    static std::string mnemonic(const Cpu &) {  return "STOP"; }
};

struct PREFIX {
    using result_type = std::size_t;

    static result_type execute(Cpu &) { return 0; }
    static std::string mnemonic(const Cpu &) {  return "PREFIX"; }
};

struct DI{
    using result_type = void;

    static void execute(Cpu &cpu) {
        cpu.bus.interrupts_master_enable_flag = false;
    }
    static std::string mnemonic(const Cpu &) {  return "DI"; }
};

struct EI{
    using result_type = void;

    static void execute(Cpu &cpu) {
        cpu.bus.interrupts_master_enable_flag = true;
    }
    static std::string mnemonic(const Cpu &) {  return "EI"; }
};

template<typename Dst, typename Src>
struct LD
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
struct JP
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
struct JR
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
struct PUSH
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        auto value = Loc::read_data(cpu);
        cpu.registers.sp -= 2;
        cpu.bus[cpu.registers.sp] = value;
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "PUSH " + Loc::to_string(cpu);
    }
};

template<typename Loc>
struct POP
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        auto value = cpu.bus[cpu.registers.sp];
        cpu.registers.sp += 2;
        Loc::write_data(cpu, value);
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "POP " + Loc::to_string(cpu);
    }
};

template<typename Cond, typename Loc>
struct CALL
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
struct RET
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
struct RST
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        cpu.registers.sp -= 2;
        cpu.bus[cpu.registers.sp] = cpu.registers.pc;

        cpu.registers.pc = Addr;
    }

    static std::string mnemonic(const Cpu &cpu) {
        std::ostringstream out;
        out << "RST " << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << Addr;
        return out.str();
    }
};

struct RETI
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
struct AND
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        auto value = Reg::read_data(cpu);

        cpu.registers.a &= value & 0xFF;
        cpu.registers.f = 0;
        cpu.registers.flags.z = cpu.registers.a == 0;
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "AND " + Reg::to_string(cpu);
    }
};

template<typename Reg>
struct OR
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
struct XOR
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

struct CPL
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

struct RLCA
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        auto value = cpu.registers.a;
        bool c_flag = (value >> 7) & 1;

        cpu.registers.f = 0;
        cpu.registers.flags.c = c_flag;

        cpu.registers.a = (value << 1) | c_flag;
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "RLCA";
    }
};

struct RRCA
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

struct RRA
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        int carry = (cpu.registers.flags.c ? 1 : 0) << 7;
        cpu.registers.f = 0;
        cpu.registers.flags.c = (cpu.registers.a & 0x01);
        cpu.registers.a = (cpu.registers.a >> 1) + carry;
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "RRA";
    }
};

template<typename Op>
struct INC
{
    using result_type = void;

    static void execute(Cpu &cpu)
    {
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
struct DEC
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
struct CP
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
struct ADD
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        auto dst_value = Dst::read_data(cpu);
        auto src_value = Src::read_data(cpu);

        std::uint32_t result32 = dst_value + src_value;

        static_assert (sizeof(dst_value) == sizeof(src_value));

        if constexpr (sizeof(Src::data_type) == 1)
        {
            std::uint8_t result = (result32 & 0xFF);

            cpu.registers.flags.z = result == 0;
            cpu.registers.flags.n = false;
            cpu.registers.flags.h = (((result & 0x0F) + (src_value & 0x0F)) > 0x0F);
            cpu.registers.flags.c = (result32 & 0xFF00);

            Dst::write_data(cpu, result);
        }
        else
        {
            cpu.registers.flags.h = (dst_value & 0xFFF)
                                  + (src_value & 0xFFF) >= 0x1000;
            cpu.registers.flags.c = result32 >= 0x10000;

            Dst::write_data(cpu, result32 & 0xFFFF);
        }


//        if (ctx->cur_inst->reg_1 == RT_SP) {
//            z = 0;
//            h = (cpu_read_reg(ctx->cur_inst->reg_1) & 0xF) + (ctx->fetched_data & 0xF) >= 0x10;
//            c = (int)(cpu_read_reg(ctx->cur_inst->reg_1) & 0xFF) + (int)(ctx->fetched_data & 0xFF) >= 0x100;
//        }
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "ADD " + Dst::to_string(cpu) + ", " + Src::to_string(cpu);
    }
};


template<typename Src>
struct SUB
{
    using result_type = void;

    static void execute(Cpu &cpu) {
        auto dst_value = A::read_data(cpu);
        auto src_value = Src::read_data(cpu);

        static_assert (sizeof(dst_value) == 1);
        static_assert (sizeof(src_value) == 1);

        std::uint16_t result = dst_value - src_value;

        cpu.registers.flags.z = result == 0;
        cpu.registers.flags.h = (int(dst_value) & 0xF) - (int(src_value) & 0xF) < 0;
        cpu.registers.flags.c = (int(dst_value))       - (int(src_value)) < 0;

        A::write_data(cpu, result & 0xFF);
    }

    static std::string mnemonic(const Cpu &cpu) {
        return "SUB " + Src::to_string(cpu);
    }
};


template<typename Val>
struct SWAP
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

template<typename Inst>
struct InstructionTraits;

template<std::uint16_t OpCode>
struct InstructionTraits<Instruction<OpCode>>
{
    using inst_type = Instruction<OpCode>;
    static constexpr std::int16_t opcode = OpCode;

    static std::string mnemonic(const Cpu &cpu)
    {
        if constexpr (inst_type::new_style)
        {
            using Impl = typename inst_type::impl_type;
            return  Impl::mnemonic(cpu);
        }
        else
        {
            return inst_type::mnemonic;
        }
    }
};


template<typename Inst>
typename std::size_t call_inst2(Cpu &cpu)
{
    if constexpr (InstructionTraits<Inst>::opcode == 0x02
               //|| InstructionTraits<Inst>::opcode == 0xC4
                 )
    {
        std::cout << "test"<< std::endl;
        _CrtDbgBreak();
        //exit(0);
    }

    using Impl = typename Inst::impl_type;
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

template<> struct Instruction<0x00> : Inst<1,  4, NOP> {};
template<> struct Instruction<0x01> : Inst<3, 12, LD<BC,d16>> {};
template<> struct Instruction<0x02> : Inst<1,  8, LD<At<BC>, A>> {};
template<> struct Instruction<0x03> : Inst<1,  8, INC<BC>> {};
template<> struct Instruction<0x04> : Inst<1,  4, INC<B>> {};
template<> struct Instruction<0x05> : Inst<1,  4, DEC<B>> {};
template<> struct Instruction<0x06> : Inst<2,  8, LD<B,d8>> {};
template<> struct Instruction<0x07> : Inst<1,  4, RLCA> {};
template<> struct Instruction<0x08> : Inst<3, 20, LD<At16<a16>,SP>> {};
template<> struct Instruction<0x09> : Inst<1,  8, ADD<HL,BC>> {};
template<> struct Instruction<0x0A> : Inst<1,  8, LD<A,At<BC>>> {};
template<> struct Instruction<0x0B> : Inst<1,  8, DEC<BC>> {};
template<> struct Instruction<0x0C> : Inst<1,  4, INC<C>> {};
template<> struct Instruction<0x0D> : Inst<1,  4, DEC<C>> {};
template<> struct Instruction<0x0E> : Inst<2,  8, LD<C,d8>> {};
template<> struct Instruction<0x0F> : Inst<1,  4, RRCA> {};


template<> struct Instruction<0x10> : Inst<2,  4, STOP> {};
template<> struct Instruction<0x11> : Inst<3, 12, LD<DE,d16>> {};
template<> struct Instruction<0x12> : Inst<1,  8, LD<At<DE>, A>> {};
template<> struct Instruction<0x13> : Inst<1,  8, INC<DE>> {};
template<> struct Instruction<0x14> : Inst<1,  4, INC<D>> {};
template<> struct Instruction<0x15> : Inst<1,  4, DEC<D>> {};
template<> struct Instruction<0x16> : Inst<2,  8, LD<D,d8>> {};
//INST(0x17, "RLA",         1,  4, nullptr };
template<> struct Instruction<0x18> : Inst<2, 12, JR<Always,r8>> {};
template<> struct Instruction<0x19> : Inst<1,  8, ADD<HL,DE>> {};
template<> struct Instruction<0x1A> : Inst<1,  8, LD<A,At<DE>>> {};
template<> struct Instruction<0x1B> : Inst<1,  8, DEC<DE>> {};
template<> struct Instruction<0x1C> : Inst<1,  4, INC<E>> {};
template<> struct Instruction<0x1D> : Inst<1,  4, DEC<E>> {};
template<> struct Instruction<0x1E> : Inst<2,  8, LD<E,d8>> {};
template<> struct Instruction<0x1F> : Inst<1,  4, RRA> {};


template<> struct Instruction<0x20> : Inst<2,  0, JR<FlagNotZ,r8>> {};
template<> struct Instruction<0x21> : Inst<3, 12, LD<HL,d16>> {};
template<> struct Instruction<0x22> : Inst<1,  8, LD<At<HL,'+'>,A>> {};
template<> struct Instruction<0x23> : Inst<1,  8, INC<HL>> {};
template<> struct Instruction<0x24> : Inst<1,  4, INC<H>> {};
template<> struct Instruction<0x25> : Inst<1,  4, DEC<H>> {};
template<> struct Instruction<0x26> : Inst<2,  8, LD<H,d8>> {};
//INST(0x27, "DAA",           1,  4, nullptr };
template<> struct Instruction<0x28> : Inst<2,  0, JR<FlagZ,r8>> {};
template<> struct Instruction<0x29> : Inst<1,  8, ADD<HL,HL>> {};
template<> struct Instruction<0x2A> : Inst<1,  8, LD<A, At<HL,'+'>>> {};
template<> struct Instruction<0x2B> : Inst<1,  8, DEC<HL>> {};
template<> struct Instruction<0x2C> : Inst<1,  4, INC<L>> {};
template<> struct Instruction<0x2D> : Inst<1,  4, DEC<L>> {};
template<> struct Instruction<0x2E> : Inst<2,  8, LD<L,d8>> {};
template<> struct Instruction<0x2F> : Inst<1,  8, CPL> {};

template<> struct Instruction<0x30> : Inst<2,  0, JR<FlagNotC,r8>> {};
template<> struct Instruction<0x31> : Inst<3, 12, LD<SP,d16>> {};
template<> struct Instruction<0x32> : Inst<1,  8, LD<At<HL,'-'>,A>> {};
template<> struct Instruction<0x33> : Inst<1,  8, INC<SP>> {};
template<> struct Instruction<0x34> : Inst<1, 12, INC<At<HL>>> {};
template<> struct Instruction<0x35> : Inst<1, 12, DEC<At<HL>>> {};
template<> struct Instruction<0x36> : Inst<2, 12, LD<At<HL>,d8>> {};
template<> struct Instruction<0x38> : Inst<2,  0, JR<FlagC,r8>> {};
template<> struct Instruction<0x39> : Inst<1,  8, ADD<HL,SP>> {};
template<> struct Instruction<0x3A> : Inst<1,  8, LD<A, At<HL,'-'>>> {};
template<> struct Instruction<0x3B> : Inst<1,  8, DEC<SP>> {};
template<> struct Instruction<0x3C> : Inst<1,  4, INC<A>> {};
template<> struct Instruction<0x3D> : Inst<1,  4, DEC<A>> {};
template<> struct Instruction<0x3E> : Inst<2,  8, LD<A,d8>> {};

template<> struct Instruction<0x40> : Inst<1, 4, LD<B,B>> {};
template<> struct Instruction<0x41> : Inst<1, 4, LD<B,C>> {};
template<> struct Instruction<0x42> : Inst<1, 4, LD<B,D>> {};
template<> struct Instruction<0x43> : Inst<1, 4, LD<B,E>> {};
template<> struct Instruction<0x44> : Inst<1, 4, LD<B,H>> {};
template<> struct Instruction<0x45> : Inst<1, 4, LD<B,L>> {};
template<> struct Instruction<0x46> : Inst<1, 8, LD<B,At<HL>>> {};
template<> struct Instruction<0x47> : Inst<1, 4, LD<B,A>> {};
template<> struct Instruction<0x48> : Inst<1, 4, LD<C,B>> {};
template<> struct Instruction<0x49> : Inst<1, 4, LD<C,C>> {};
template<> struct Instruction<0x4A> : Inst<1, 4, LD<C,D>> {};
template<> struct Instruction<0x4B> : Inst<1, 4, LD<C,E>> {};
template<> struct Instruction<0x4C> : Inst<1, 4, LD<C,H>> {};
template<> struct Instruction<0x4D> : Inst<1, 4, LD<C,L>> {};
template<> struct Instruction<0x4E> : Inst<1, 8, LD<C,At<HL>>> {};
template<> struct Instruction<0x4F> : Inst<1, 4, LD<C,A>> {};

template<> struct Instruction<0x50> : Inst<1, 4, LD<D,B>> {};
template<> struct Instruction<0x51> : Inst<1, 4, LD<D,C>> {};
template<> struct Instruction<0x52> : Inst<1, 4, LD<D,D>> {};
template<> struct Instruction<0x53> : Inst<1, 4, LD<D,E>> {};
template<> struct Instruction<0x54> : Inst<1, 4, LD<D,H>> {};
template<> struct Instruction<0x55> : Inst<1, 4, LD<D,L>> {};
template<> struct Instruction<0x56> : Inst<1, 8, LD<D,At<HL>>> {};
template<> struct Instruction<0x57> : Inst<1, 4, LD<D,A>> {};
template<> struct Instruction<0x58> : Inst<1, 4, LD<E,B>> {};
template<> struct Instruction<0x59> : Inst<1, 4, LD<E,C>> {};
template<> struct Instruction<0x5A> : Inst<1, 4, LD<E,D>> {};
template<> struct Instruction<0x5B> : Inst<1, 4, LD<E,E>> {};
template<> struct Instruction<0x5C> : Inst<1, 4, LD<E,H>> {};
template<> struct Instruction<0x5D> : Inst<1, 4, LD<E,L>> {};
template<> struct Instruction<0x5E> : Inst<1, 8, LD<E,At<HL>>> {};
template<> struct Instruction<0x5F> : Inst<1, 4, LD<E,A>> {};

template<> struct Instruction<0x60> : Inst<1, 4, LD<H,B>> {};
template<> struct Instruction<0x61> : Inst<1, 4, LD<H,C>> {};
template<> struct Instruction<0x62> : Inst<1, 4, LD<H,D>> {};
template<> struct Instruction<0x63> : Inst<1, 4, LD<H,E>> {};
template<> struct Instruction<0x64> : Inst<1, 4, LD<H,H>> {};
template<> struct Instruction<0x65> : Inst<1, 4, LD<H,L>> {};
template<> struct Instruction<0x66> : Inst<1, 8, LD<H,At<HL>>> {};
template<> struct Instruction<0x67> : Inst<1, 4, LD<H,A>> {};
template<> struct Instruction<0x68> : Inst<1, 4, LD<L,B>> {};
template<> struct Instruction<0x69> : Inst<1, 4, LD<L,C>> {};
template<> struct Instruction<0x6A> : Inst<1, 4, LD<L,D>> {};
template<> struct Instruction<0x6B> : Inst<1, 4, LD<L,E>> {};
template<> struct Instruction<0x6C> : Inst<1, 4, LD<L,H>> {};
template<> struct Instruction<0x6D> : Inst<1, 4, LD<L,L>> {};
template<> struct Instruction<0x6E> : Inst<1, 8, LD<L,At<HL>>> {};
template<> struct Instruction<0x6F> : Inst<1, 4, LD<L,A>> {};

template<> struct Instruction<0x70> : Inst<1, 8, LD<At<HL>,B>> {};
template<> struct Instruction<0x71> : Inst<1, 8, LD<At<HL>,C>> {};
template<> struct Instruction<0x72> : Inst<1, 8, LD<At<HL>,D>> {};
template<> struct Instruction<0x73> : Inst<1, 8, LD<At<HL>,E>> {};
template<> struct Instruction<0x74> : Inst<1, 8, LD<At<HL>,H>> {};
template<> struct Instruction<0x75> : Inst<1, 8, LD<At<HL>,L>> {};
template<> struct Instruction<0x77> : Inst<1, 8, LD<At<HL>,A>> {};
template<> struct Instruction<0x78> : Inst<1, 4, LD<A,B>> {};
template<> struct Instruction<0x79> : Inst<1, 4, LD<A,C>> {};
template<> struct Instruction<0x7A> : Inst<1, 4, LD<A,D>> {};
template<> struct Instruction<0x7B> : Inst<1, 4, LD<A,E>> {};
template<> struct Instruction<0x7C> : Inst<1, 4, LD<A,H>> {};
template<> struct Instruction<0x7D> : Inst<1, 4, LD<A,L>> {};
template<> struct Instruction<0x7E> : Inst<1, 8, LD<A,At<HL>>> {};
template<> struct Instruction<0x7F> : Inst<1, 4, LD<A,A>> {};

template<> struct Instruction<0x80> : Inst<1, 4, ADD<A,B>> {};
template<> struct Instruction<0x81> : Inst<1, 4, ADD<A,C>> {};
template<> struct Instruction<0x82> : Inst<1, 4, ADD<A,D>> {};
template<> struct Instruction<0x83> : Inst<1, 4, ADD<A,E>> {};
template<> struct Instruction<0x84> : Inst<1, 4, ADD<A,H>> {};
template<> struct Instruction<0x85> : Inst<1, 4, ADD<A,L>> {};
template<> struct Instruction<0x86> : Inst<1, 8, ADD<A,At<HL>>> {};
template<> struct Instruction<0x87> : Inst<1, 4, ADD<A,A>> {};
INST(0x88, "ADC A, B",    1,  4, Cpu &cpu){ op_adc(cpu, cpu.registers.a, cpu.registers.b); }
INST(0x89, "ADC A, C",    1,  4, Cpu &cpu){ op_adc(cpu, cpu.registers.a, cpu.registers.c); }
INST(0x8A, "ADC A, D",    1,  4, Cpu &cpu){ op_adc(cpu, cpu.registers.a, cpu.registers.d); }
INST(0x8B, "ADC A, E",    1,  4, Cpu &cpu){ op_adc(cpu, cpu.registers.a, cpu.registers.e); }
INST(0x8C, "ADC A, H",    1,  4, Cpu &cpu){ op_adc(cpu, cpu.registers.a, cpu.registers.h); }
INST(0x8D, "ADC A, L",    1,  4, Cpu &cpu){ op_adc(cpu, cpu.registers.a, cpu.registers.l); }
INST(0x8E, "ADC A, (HL)", 1,  8, Cpu &cpu){ op_adc(cpu, cpu.registers.a, cpu.bus[cpu.registers.hl]); }
INST(0x8F, "ADC A, A",    1,  4, Cpu &cpu){ op_adc(cpu, cpu.registers.a, cpu.registers.a); }

template<> struct Instruction<0x91> : Inst<1, 4, SUB<C>> {};

template<> struct Instruction<0xA0> : Inst<1,  4, AND<B>> {};
template<> struct Instruction<0xA1> : Inst<1,  4, AND<C>> {};
template<> struct Instruction<0xA2> : Inst<1,  4, AND<D>> {};
template<> struct Instruction<0xA3> : Inst<1,  4, AND<E>> {};
template<> struct Instruction<0xA4> : Inst<1,  4, AND<H>> {};
template<> struct Instruction<0xA5> : Inst<1,  4, AND<L>> {};
template<> struct Instruction<0xA6> : Inst<1,  8, AND<At<HL>>> {};
template<> struct Instruction<0xA7> : Inst<1,  4, AND<A>> {};
template<> struct Instruction<0xA8> : Inst<1,  4, XOR<B>> {};
template<> struct Instruction<0xA9> : Inst<1,  4, XOR<C>> {};
template<> struct Instruction<0xAA> : Inst<1,  4, XOR<D>> {};
template<> struct Instruction<0xAB> : Inst<1,  4, XOR<E>> {};
template<> struct Instruction<0xAC> : Inst<1,  4, XOR<H>> {};
template<> struct Instruction<0xAD> : Inst<1,  4, XOR<L>> {};
template<> struct Instruction<0xAE> : Inst<1,  8, XOR<At<HL>>> {};
template<> struct Instruction<0xAF> : Inst<1,  4, XOR<A>> {};

template<> struct Instruction<0xB0> : Inst<1,  4, OR<B>> {};
template<> struct Instruction<0xB1> : Inst<1,  4, OR<C>> {};
template<> struct Instruction<0xB2> : Inst<1,  4, OR<D>> {};
template<> struct Instruction<0xB3> : Inst<1,  4, OR<E>> {};
template<> struct Instruction<0xB4> : Inst<1,  4, OR<H>> {};
template<> struct Instruction<0xB5> : Inst<1,  4, OR<L>> {};
template<> struct Instruction<0xB6> : Inst<1,  4, OR<At<HL>>> {};
template<> struct Instruction<0xB7> : Inst<1,  4, OR<A>> {};
template<> struct Instruction<0xB8> : Inst<1,  4, CP<B>> {};
template<> struct Instruction<0xB9> : Inst<1,  4, CP<C>> {};
template<> struct Instruction<0xBA> : Inst<1,  4, CP<D>> {};
template<> struct Instruction<0xBB> : Inst<1,  4, CP<E>> {};
template<> struct Instruction<0xBC> : Inst<1,  4, CP<H>> {};
template<> struct Instruction<0xBD> : Inst<1,  4, CP<L>> {};
template<> struct Instruction<0xBE> : Inst<1,  8, CP<At<HL>>> {};
template<> struct Instruction<0xBF> : Inst<1,  4, CP<A>> {};

template<> struct Instruction<0xC0> : Inst<1,  0, RET<FlagNotZ>> {};
template<> struct Instruction<0xC1> : Inst<1, 12, POP<BC>> {};
template<> struct Instruction<0xC2> : Inst<3,  0, JP<FlagNotZ,a16>> {};
template<> struct Instruction<0xC3> : Inst<3, 16, JP<Always,a16>> {};
template<> struct Instruction<0xC4> : Inst<3,  0, CALL<FlagNotZ,a16>> {};
template<> struct Instruction<0xC5> : Inst<1, 16, PUSH<BC>> {};
template<> struct Instruction<0xC6> : Inst<2,  8, ADD<A,d8>> {};
template<> struct Instruction<0xC7> : Inst<1, 16, RST<0x00>> {};
template<> struct Instruction<0xC8> : Inst<1,  0, RET<FlagZ>> {};
template<> struct Instruction<0xC9> : Inst<1, 16, RET<>> {};
template<> struct Instruction<0xCA> : Inst<3,  0, JP<FlagZ,a16>> {};
template<> struct Instruction<0xCB> : Inst<2,  0, PREFIX> {};
template<> struct Instruction<0xCC> : Inst<3,  0, CALL<FlagZ,a16>> {};
template<> struct Instruction<0xCD> : Inst<3, 24, CALL<Always,a16>> {};
template<> struct Instruction<0xCE> : Inst<2,  8, ADD<A, d8>> {};
template<> struct Instruction<0xCF> : Inst<1, 16, RST<0x08>> {};

template<> struct Instruction<0xD0> : Inst<1,  0, RET<FlagNotC>> {};
template<> struct Instruction<0xD1> : Inst<1, 12, POP<DE>> {};
template<> struct Instruction<0xD2> : Inst<3,  0, JP<FlagNotC,a16>> {};
template<> struct Instruction<0xD4> : Inst<3,  0, CALL<FlagNotC,a16>> {};
template<> struct Instruction<0xD5> : Inst<1, 16, PUSH<DE>> {};
template<> struct Instruction<0xD6> : Inst<2,  8, SUB<d8>> {};
template<> struct Instruction<0xD7> : Inst<1, 16, RST<0x10>> {};
template<> struct Instruction<0xD8> : Inst<1,  0, RET<FlagC>> {};
template<> struct Instruction<0xD9> : Inst<1, 16, RETI> {};
template<> struct Instruction<0xDA> : Inst<3,  0, JP<FlagC,a16>> {};
template<> struct Instruction<0xDC> : Inst<3,  0, CALL<FlagC,a16>> {};
template<> struct Instruction<0xDF> : Inst<1, 16, RST<0x18>> {};

template<> struct Instruction<0xE0> : Inst<2, 12, LD<At<a8>, A>> {};
template<> struct Instruction<0xE1> : Inst<1, 12, POP<HL>> {};
template<> struct Instruction<0xE2> : Inst<1,  8, LD<At<C>,A>> {};
template<> struct Instruction<0xE5> : Inst<1, 16, PUSH<HL>> {};
template<> struct Instruction<0xE6> : Inst<2,  8, AND<d8>> {};
template<> struct Instruction<0xE7> : Inst<1, 16, RST<0x20>> {};
template<> struct Instruction<0xE9> : Inst<1,  4, JP<Always,HL>> {};
template<> struct Instruction<0xEA> : Inst<3, 12, LD<At<a16>, A>> {};
template<> struct Instruction<0xEE> : Inst<2,  8, XOR<d8>> {};
template<> struct Instruction<0xEF> : Inst<1, 16, RST<0x28>> {};

template<> struct Instruction<0xF0> : Inst<2, 12, LD<A, At<a8>>> {};
template<> struct Instruction<0xF1> : Inst<1, 12, POP<AF>> {};
template<> struct Instruction<0xF3> : Inst<1,  4, DI> {};
template<> struct Instruction<0xF5> : Inst<1, 16, PUSH<AF>> {};
template<> struct Instruction<0xF6> : Inst<2,  8, OR<d8>> {};
template<> struct Instruction<0xF7> : Inst<1, 16, RST<0x30>> {};
template<> struct Instruction<0xF8> : Inst<2, 12, LD<HL,SP_r8>> {};//0 0 H C
template<> struct Instruction<0xF9> : Inst<1,  8, LD<SP, HL>> {};
template<> struct Instruction<0xFA> : Inst<3, 16, LD<A,At<a16>>> {};
template<> struct Instruction<0xFB> : Inst<1,  4, EI> {};
template<> struct Instruction<0xFE> : Inst<2,  8, CP<d8>> {};
template<> struct Instruction<0xFF> : Inst<1, 16, RST<0x38>> {};


INST(0xCB20, "SLA B",    2,   8, Cpu &cpu) { op_sla(cpu, cpu.registers.b); }
INST(0xCB21, "SLA C",    2,   8, Cpu &cpu) { op_sla(cpu, cpu.registers.c); }
INST(0xCB22, "SLA D",    2,   8, Cpu &cpu) { op_sla(cpu, cpu.registers.d); }
INST(0xCB23, "SLA E",    2,   8, Cpu &cpu) { op_sla(cpu, cpu.registers.e); }
INST(0xCB24, "SLA H",    2,   8, Cpu &cpu) { op_sla(cpu, cpu.registers.h); }
INST(0xCB25, "SLA L",    2,   8, Cpu &cpu) { op_sla(cpu, cpu.registers.l); }
INST(0xCB26, "SLA (HL)", 2,  16, Cpu &cpu) { auto ref = cpu.bus[cpu.registers.hl]; op_sla(cpu, ref); }
INST(0xCB27, "SLA A",    2,   8, Cpu &cpu) { op_sla(cpu, cpu.registers.a); }

INST(0xCB30, "SWAP B",    2,  8, Cpu &cpu) { cpu.registers.b = op_swap(cpu, cpu.registers.b); }
INST(0xCB31, "SWAP C",    2,  8, Cpu &cpu) { cpu.registers.c = op_swap(cpu, cpu.registers.c); }
INST(0xCB32, "SWAP D",    2,  8, Cpu &cpu) { cpu.registers.d = op_swap(cpu, cpu.registers.d); }
INST(0xCB33, "SWAP E",    2,  8, Cpu &cpu) { cpu.registers.e = op_swap(cpu, cpu.registers.e); }
INST(0xCB34, "SWAP H",    2,  8, Cpu &cpu) { cpu.registers.h = op_swap(cpu, cpu.registers.h); }
INST(0xCB35, "SWAP L",    2,  8, Cpu &cpu) { cpu.registers.l = op_swap(cpu, cpu.registers.l); }
//INST(0xCB36, "SWAP (HL)", 2, 16, Cpu &cpu) { cpu.bus[cpu.registers.hl] = op_swap(cpu, cpu.bus[cpu.registers.hl]); }
//INST(0xCB37, "SWAP A",    2,  8, Cpu &cpu) { cpu.registers.a = op_swap(cpu, cpu.registers.a); }
template<> struct Instruction<0xCB37> : Inst<2,  8, SWAP<A>> {};

INST(0xCB40, "BIT 0, B",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 0, cpu.registers.b); }
INST(0xCB41, "BIT 0, C",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 0, cpu.registers.c); }
INST(0xCB42, "BIT 0, D",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 0, cpu.registers.d); }
INST(0xCB43, "BIT 0, E",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 0, cpu.registers.e); }
INST(0xCB44, "BIT 0, H",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 0, cpu.registers.h); }
INST(0xCB45, "BIT 0, L",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 0, cpu.registers.l); }
INST(0xCB46, "BIT 0, (HL)", 2,  12, Cpu &cpu) { op_bit(cpu, 1 << 0, cpu.bus[cpu.registers.hl]); }
INST(0xCB47, "BIT 0, A",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 0, cpu.registers.a); }
INST(0xCB48, "BIT 1, B",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 1, cpu.registers.b); }
INST(0xCB49, "BIT 1, C",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 1, cpu.registers.c); }
INST(0xCB4A, "BIT 1, D",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 1, cpu.registers.d); }
INST(0xCB4B, "BIT 1, E",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 1, cpu.registers.e); }
INST(0xCB4C, "BIT 1, H",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 1, cpu.registers.h); }
INST(0xCB4D, "BIT 1, L",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 1, cpu.registers.l); }
INST(0xCB4E, "BIT 1, (HL)", 2,  12, Cpu &cpu) { op_bit(cpu, 1 << 1, cpu.bus[cpu.registers.hl]); }
INST(0xCB4F, "BIT 1, A",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 1, cpu.registers.a); }

INST(0xCB50, "BIT 2, B",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 2, cpu.registers.b); }
INST(0xCB51, "BIT 2, C",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 2, cpu.registers.c); }
INST(0xCB52, "BIT 2, D",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 2, cpu.registers.d); }
INST(0xCB53, "BIT 2, E",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 2, cpu.registers.e); }
INST(0xCB54, "BIT 2, H",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 2, cpu.registers.h); }
INST(0xCB55, "BIT 2, L",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 2, cpu.registers.l); }
INST(0xCB56, "BIT 2, (HL)", 2,  12, Cpu &cpu) { op_bit(cpu, 1 << 2, cpu.bus[cpu.registers.hl]); }
INST(0xCB57, "BIT 2, A",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 2, cpu.registers.a); }
INST(0xCB58, "BIT 3, B",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 3, cpu.registers.b); }
INST(0xCB59, "BIT 3, C",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 3, cpu.registers.c); }
INST(0xCB5A, "BIT 3, D",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 3, cpu.registers.d); }
INST(0xCB5B, "BIT 3, E",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 3, cpu.registers.e); }
INST(0xCB5C, "BIT 3, H",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 3, cpu.registers.h); }
INST(0xCB5D, "BIT 3, L",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 3, cpu.registers.l); }
INST(0xCB5E, "BIT 3, (HL)", 2,  12, Cpu &cpu) { op_bit(cpu, 1 << 3, cpu.bus[cpu.registers.hl]); }
INST(0xCB5F, "BIT 3, A",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 3, cpu.registers.a); }

INST(0xCB60, "BIT 4, B",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 4, cpu.registers.b); }
INST(0xCB61, "BIT 4, C",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 4, cpu.registers.c); }
INST(0xCB62, "BIT 4, D",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 4, cpu.registers.d); }
INST(0xCB63, "BIT 4, E",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 4, cpu.registers.e); }
INST(0xCB64, "BIT 4, H",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 4, cpu.registers.h); }
INST(0xCB65, "BIT 4, L",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 4, cpu.registers.l); }
INST(0xCB66, "BIT 4, (HL)", 2,  12, Cpu &cpu) { op_bit(cpu, 1 << 4, cpu.bus[cpu.registers.hl]); }
INST(0xCB67, "BIT 4, A",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 4, cpu.registers.a); }
INST(0xCB68, "BIT 5, B",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 5, cpu.registers.b); }
INST(0xCB69, "BIT 5, C",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 5, cpu.registers.c); }
INST(0xCB6A, "BIT 5, D",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 5, cpu.registers.d); }
INST(0xCB6B, "BIT 5, E",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 5, cpu.registers.e); }
INST(0xCB6C, "BIT 5, H",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 5, cpu.registers.h); }
INST(0xCB6D, "BIT 5, L",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 5, cpu.registers.l); }
INST(0xCB6E, "BIT 5, (HL)", 2,  12, Cpu &cpu) { op_bit(cpu, 1 << 5, cpu.bus[cpu.registers.hl]); }
INST(0xCB6F, "BIT 5, A",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 5, cpu.registers.a); }

INST(0xCB70, "BIT 6, B",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 6, cpu.registers.b); }
INST(0xCB71, "BIT 6, C",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 6, cpu.registers.c); }
INST(0xCB72, "BIT 6, D",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 6, cpu.registers.d); }
INST(0xCB73, "BIT 6, E",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 6, cpu.registers.e); }
INST(0xCB74, "BIT 6, H",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 6, cpu.registers.h); }
INST(0xCB75, "BIT 6, L",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 6, cpu.registers.l); }
INST(0xCB76, "BIT 6, (HL)", 2,  12, Cpu &cpu) { op_bit(cpu, 1 << 6, cpu.bus[cpu.registers.hl]); }
INST(0xCB77, "BIT 6, A",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 6, cpu.registers.a); }
INST(0xCB78, "BIT 7, B",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 7, cpu.registers.b); }
INST(0xCB79, "BIT 7, C",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 7, cpu.registers.c); }
INST(0xCB7A, "BIT 7, D",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 7, cpu.registers.d); }
INST(0xCB7B, "BIT 7, E",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 7, cpu.registers.e); }
INST(0xCB7C, "BIT 7, H",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 7, cpu.registers.h); }
INST(0xCB7D, "BIT 7, L",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 7, cpu.registers.l); }
INST(0xCB7E, "BIT 7, (HL)", 2,  12, Cpu &cpu) { op_bit(cpu, 1 << 7, cpu.bus[cpu.registers.hl]); }
INST(0xCB7F, "BIT 7, A",    2,   8, Cpu &cpu) { op_bit(cpu, 1 << 7, cpu.registers.a); }

INST(0xCB80, "RES 0, B",    2,   8, Cpu &cpu) { op_res(0, cpu.registers.b); }
INST(0xCB81, "RES 0, C",    2,   8, Cpu &cpu) { op_res(0, cpu.registers.c); }
INST(0xCB82, "RES 0, D",    2,   8, Cpu &cpu) { op_res(0, cpu.registers.d); }
INST(0xCB83, "RES 0, E",    2,   8, Cpu &cpu) { op_res(0, cpu.registers.e); }
INST(0xCB84, "RES 0, H",    2,   8, Cpu &cpu) { op_res(0, cpu.registers.h); }
INST(0xCB85, "RES 0, L",    2,   8, Cpu &cpu) { op_res(0, cpu.registers.l); }
INST(0xCB86, "RES 0, (HL)", 2,  16, Cpu &cpu) { auto ref = cpu.bus[cpu.registers.hl]; op_res(0, ref); }
INST(0xCB87, "RES 0, A",    2,   8, Cpu &cpu) { op_res(0, cpu.registers.a); }
INST(0xCB88, "RES 1, B",    2,   8, Cpu &cpu) { op_res(1, cpu.registers.b); }
INST(0xCB89, "RES 1, C",    2,   8, Cpu &cpu) { op_res(1, cpu.registers.c); }
INST(0xCB8A, "RES 1, D",    2,   8, Cpu &cpu) { op_res(1, cpu.registers.d); }
INST(0xCB8B, "RES 1, E",    2,   8, Cpu &cpu) { op_res(1, cpu.registers.e); }
INST(0xCB8C, "RES 1, H",    2,   8, Cpu &cpu) { op_res(1, cpu.registers.h); }
INST(0xCB8D, "RES 1, L",    2,   8, Cpu &cpu) { op_res(1, cpu.registers.l); }
INST(0xCB8E, "RES 1, (HL)", 2,  16, Cpu &cpu) { auto ref = cpu.bus[cpu.registers.hl]; op_res(1, ref); }
INST(0xCB8F, "RES 1, A",    2,   8, Cpu &cpu) { op_res(1, cpu.registers.a); }

INST(0xCB90, "RES 2, B",    2,   8, Cpu &cpu) { op_res(2, cpu.registers.b); }
INST(0xCB91, "RES 2, C",    2,   8, Cpu &cpu) { op_res(2, cpu.registers.c); }
INST(0xCB92, "RES 2, D",    2,   8, Cpu &cpu) { op_res(2, cpu.registers.d); }
INST(0xCB93, "RES 2, E",    2,   8, Cpu &cpu) { op_res(2, cpu.registers.e); }
INST(0xCB94, "RES 2, H",    2,   8, Cpu &cpu) { op_res(2, cpu.registers.h); }
INST(0xCB95, "RES 2, L",    2,   8, Cpu &cpu) { op_res(2, cpu.registers.l); }
INST(0xCB96, "RES 2, (HL)", 2,  16, Cpu &cpu) { auto ref = cpu.bus[cpu.registers.hl]; op_res(2, ref); }
INST(0xCB97, "RES 2, A",    2,   8, Cpu &cpu) { op_res(2, cpu.registers.a); }
INST(0xCB98, "RES 3, B",    2,   8, Cpu &cpu) { op_res(3, cpu.registers.b); }
INST(0xCB99, "RES 3, C",    2,   8, Cpu &cpu) { op_res(3, cpu.registers.c); }
INST(0xCB9A, "RES 3, D",    2,   8, Cpu &cpu) { op_res(3, cpu.registers.d); }
INST(0xCB9B, "RES 3, E",    2,   8, Cpu &cpu) { op_res(3, cpu.registers.e); }
INST(0xCB9C, "RES 3, H",    2,   8, Cpu &cpu) { op_res(3, cpu.registers.h); }
INST(0xCB9D, "RES 3, L",    2,   8, Cpu &cpu) { op_res(3, cpu.registers.l); }
INST(0xCB9E, "RES 3, (HL)", 2,  16, Cpu &cpu) { auto ref = cpu.bus[cpu.registers.hl]; op_res(3, ref); }
INST(0xCB9F, "RES 3, A",    2,   8, Cpu &cpu) { op_res(3, cpu.registers.a); }

INST(0xCBA0, "RES 4, B",    2,   8, Cpu &cpu) { op_res(4, cpu.registers.b); }
INST(0xCBA1, "RES 4, C",    2,   8, Cpu &cpu) { op_res(4, cpu.registers.c); }
INST(0xCBA2, "RES 4, D",    2,   8, Cpu &cpu) { op_res(4, cpu.registers.d); }
INST(0xCBA3, "RES 4, E",    2,   8, Cpu &cpu) { op_res(4, cpu.registers.e); }
INST(0xCBA4, "RES 4, H",    2,   8, Cpu &cpu) { op_res(4, cpu.registers.h); }
INST(0xCBA5, "RES 4, L",    2,   8, Cpu &cpu) { op_res(4, cpu.registers.l); }
INST(0xCBA6, "RES 4, (HL)", 2,  16, Cpu &cpu) { auto ref = cpu.bus[cpu.registers.hl]; op_res(4, ref); }
INST(0xCBA7, "RES 4, A",    2,   8, Cpu &cpu) { op_res(4, cpu.registers.a); }
INST(0xCBA8, "RES 5, B",    2,   8, Cpu &cpu) { op_res(5, cpu.registers.b); }
INST(0xCBA9, "RES 5, C",    2,   8, Cpu &cpu) { op_res(5, cpu.registers.c); }
INST(0xCBAA, "RES 5, D",    2,   8, Cpu &cpu) { op_res(5, cpu.registers.d); }
INST(0xCBAB, "RES 5, E",    2,   8, Cpu &cpu) { op_res(5, cpu.registers.e); }
INST(0xCBAC, "RES 5, H",    2,   8, Cpu &cpu) { op_res(5, cpu.registers.h); }
INST(0xCBAD, "RES 5, L",    2,   8, Cpu &cpu) { op_res(5, cpu.registers.l); }
INST(0xCBAE, "RES 5, (HL)", 2,  16, Cpu &cpu) { auto ref = cpu.bus[cpu.registers.hl]; op_res(5, ref); }
INST(0xCBAF, "RES 5, A",    2,   8, Cpu &cpu) { op_res(5, cpu.registers.a); }

INST(0xCBB0, "RES 6, B",    2,   8, Cpu &cpu) { op_res(6, cpu.registers.b); }
INST(0xCBB1, "RES 6, C",    2,   8, Cpu &cpu) { op_res(6, cpu.registers.c); }
INST(0xCBB2, "RES 6, D",    2,   8, Cpu &cpu) { op_res(6, cpu.registers.d); }
INST(0xCBB3, "RES 6, E",    2,   8, Cpu &cpu) { op_res(6, cpu.registers.e); }
INST(0xCBB4, "RES 6, H",    2,   8, Cpu &cpu) { op_res(6, cpu.registers.h); }
INST(0xCBB5, "RES 6, L",    2,   8, Cpu &cpu) { op_res(6, cpu.registers.l); }
INST(0xCBB6, "RES 6, (HL)", 2,  16, Cpu &cpu) { auto ref = cpu.bus[cpu.registers.hl]; op_res(6, ref); }
INST(0xCBB7, "RES 6, A",    2,   8, Cpu &cpu) { op_res(6, cpu.registers.a); }
INST(0xCBB8, "RES 7, B",    2,   8, Cpu &cpu) { op_res(7, cpu.registers.b); }
INST(0xCBB9, "RES 7, C",    2,   8, Cpu &cpu) { op_res(7, cpu.registers.c); }
INST(0xCBBA, "RES 7, D",    2,   8, Cpu &cpu) { op_res(7, cpu.registers.d); }
INST(0xCBBB, "RES 7, E",    2,   8, Cpu &cpu) { op_res(7, cpu.registers.e); }
INST(0xCBBC, "RES 7, H",    2,   8, Cpu &cpu) { op_res(7, cpu.registers.h); }
INST(0xCBBD, "RES 7, L",    2,   8, Cpu &cpu) { op_res(7, cpu.registers.l); }
INST(0xCBBE, "RES 7, (HL)", 2,  16, Cpu &cpu) { auto ref = cpu.bus[cpu.registers.hl]; op_res(7, ref); }
INST(0xCBBF, "RES 7, A",    2,   8, Cpu &cpu) { op_res(7, cpu.registers.a); }

INST(0xCBC0, "SET 2, B",    2,   8, Cpu &cpu) { op_set(0, cpu.registers.b); }
INST(0xCBC1, "SET 2, C",    2,   8, Cpu &cpu) { op_set(0, cpu.registers.c); }
INST(0xCBC2, "SET 2, D",    2,   8, Cpu &cpu) { op_set(0, cpu.registers.d); }
INST(0xCBC3, "SET 2, E",    2,   8, Cpu &cpu) { op_set(0, cpu.registers.e); }
INST(0xCBC4, "SET 2, H",    2,   8, Cpu &cpu) { op_set(0, cpu.registers.f); }
INST(0xCBC5, "SET 2, L",    2,   8, Cpu &cpu) { op_set(0, cpu.registers.l); }
INST(0xCBC6, "SET 2, (HL)", 2,  16, Cpu &cpu) { auto ref = cpu.bus[cpu.registers.hl]; op_set(0, ref); }
INST(0xCBC7, "SET 2, A",    2,   8, Cpu &cpu) { op_set(0, cpu.registers.a); }
INST(0xCBC8, "SET 3, B",    2,   8, Cpu &cpu) { op_set(1, cpu.registers.b); }
INST(0xCBC9, "SET 3, C",    2,   8, Cpu &cpu) { op_set(1, cpu.registers.c); }
INST(0xCBCA, "SET 3, D",    2,   8, Cpu &cpu) { op_set(1, cpu.registers.d); }
INST(0xCBCB, "SET 3, E",    2,   8, Cpu &cpu) { op_set(1, cpu.registers.e); }
INST(0xCBCC, "SET 3, H",    2,   8, Cpu &cpu) { op_set(1, cpu.registers.h); }
INST(0xCBCD, "SET 3, L",    2,   8, Cpu &cpu) { op_set(1, cpu.registers.l); }
INST(0xCBCE, "SET 3, (HL)", 2,  16, Cpu &cpu) { auto ref = cpu.bus[cpu.registers.hl]; op_set(1, ref); }
INST(0xCBCF, "SET 3, A",    2,   8, Cpu &cpu) { op_set(1, cpu.registers.a); }

INST(0xCBD0, "SET 2, B",    2,   8, Cpu &cpu) { op_set(2, cpu.registers.b); }
INST(0xCBD1, "SET 2, C",    2,   8, Cpu &cpu) { op_set(2, cpu.registers.c); }
INST(0xCBD2, "SET 2, D",    2,   8, Cpu &cpu) { op_set(2, cpu.registers.d); }
INST(0xCBD3, "SET 2, E",    2,   8, Cpu &cpu) { op_set(2, cpu.registers.e); }
INST(0xCBD4, "SET 2, H",    2,   8, Cpu &cpu) { op_set(2, cpu.registers.f); }
INST(0xCBD5, "SET 2, L",    2,   8, Cpu &cpu) { op_set(2, cpu.registers.l); }
INST(0xCBD6, "SET 2, (HL)", 2,  16, Cpu &cpu) { auto ref = cpu.bus[cpu.registers.hl]; op_set(2, ref); }
INST(0xCBD7, "SET 2, A",    2,   8, Cpu &cpu) { op_set(2, cpu.registers.a); }
INST(0xCBD8, "SET 3, B",    2,   8, Cpu &cpu) { op_set(3, cpu.registers.b); }
INST(0xCBD9, "SET 3, C",    2,   8, Cpu &cpu) { op_set(3, cpu.registers.c); }
INST(0xCBDA, "SET 3, D",    2,   8, Cpu &cpu) { op_set(3, cpu.registers.d); }
INST(0xCBDB, "SET 3, E",    2,   8, Cpu &cpu) { op_set(3, cpu.registers.e); }
INST(0xCBDC, "SET 3, H",    2,   8, Cpu &cpu) { op_set(3, cpu.registers.h); }
INST(0xCBDD, "SET 3, L",    2,   8, Cpu &cpu) { op_set(3, cpu.registers.l); }
INST(0xCBDE, "SET 3, (HL)", 2,  16, Cpu &cpu) { auto ref = cpu.bus[cpu.registers.hl]; op_set(3, ref); }
INST(0xCBDF, "SET 3, A",    2,   8, Cpu &cpu) { op_set(3, cpu.registers.a); }

INST(0xCBE0, "SET 4, B",    2,   8, Cpu &cpu) { op_set(4, cpu.registers.b); }
INST(0xCBE1, "SET 4, C",    2,   8, Cpu &cpu) { op_set(4, cpu.registers.c); }
INST(0xCBE2, "SET 4, D",    2,   8, Cpu &cpu) { op_set(4, cpu.registers.d); }
INST(0xCBE3, "SET 4, E",    2,   8, Cpu &cpu) { op_set(4, cpu.registers.e); }
INST(0xCBE4, "SET 4, H",    2,   8, Cpu &cpu) { op_set(4, cpu.registers.f); }
INST(0xCBE5, "SET 4, L",    2,   8, Cpu &cpu) { op_set(4, cpu.registers.l); }
INST(0xCBE6, "SET 4, (HL)", 2,  16, Cpu &cpu) { auto ref = cpu.bus[cpu.registers.hl]; op_set(4, ref); }
INST(0xCBE7, "SET 4, A",    2,   8, Cpu &cpu) { op_set(4, cpu.registers.a); }
INST(0xCBE8, "SET 5, B",    2,   8, Cpu &cpu) { op_set(5, cpu.registers.b); }
INST(0xCBE9, "SET 5, C",    2,   8, Cpu &cpu) { op_set(5, cpu.registers.c); }
INST(0xCBEA, "SET 5, D",    2,   8, Cpu &cpu) { op_set(5, cpu.registers.d); }
INST(0xCBEB, "SET 5, E",    2,   8, Cpu &cpu) { op_set(5, cpu.registers.e); }
INST(0xCBEC, "SET 5, H",    2,   8, Cpu &cpu) { op_set(5, cpu.registers.h); }
INST(0xCBED, "SET 5, L",    2,   8, Cpu &cpu) { op_set(5, cpu.registers.l); }
INST(0xCBEE, "SET 5, (HL)", 2,  16, Cpu &cpu) { auto ref = cpu.bus[cpu.registers.hl]; op_set(5, ref); }
INST(0xCBEF, "SET 5, A",    2,   8, Cpu &cpu) { op_set(5, cpu.registers.a); }

INST(0xCBF0, "SET 6, B",    2,   8, Cpu &cpu) { op_set(6, cpu.registers.b); }
INST(0xCBF1, "SET 6, C",    2,   8, Cpu &cpu) { op_set(6, cpu.registers.c); }
INST(0xCBF2, "SET 6, D",    2,   8, Cpu &cpu) { op_set(6, cpu.registers.d); }
INST(0xCBF3, "SET 6, E",    2,   8, Cpu &cpu) { op_set(6, cpu.registers.e); }
INST(0xCBF4, "SET 6, H",    2,   8, Cpu &cpu) { op_set(6, cpu.registers.f); }
INST(0xCBF5, "SET 6, L",    2,   8, Cpu &cpu) { op_set(6, cpu.registers.l); }
INST(0xCBF6, "SET 6, (HL)", 2,  16, Cpu &cpu) { auto ref = cpu.bus[cpu.registers.hl]; op_set(6, ref); }
INST(0xCBF7, "SET 6, A",    2,   8, Cpu &cpu) { op_set(6, cpu.registers.a); }
INST(0xCBF8, "SET 7, B",    2,   8, Cpu &cpu) { op_set(7, cpu.registers.b); }
INST(0xCBF9, "SET 7, C",    2,   8, Cpu &cpu) { op_set(7, cpu.registers.c); }
INST(0xCBFA, "SET 7, D",    2,   8, Cpu &cpu) { op_set(7, cpu.registers.d); }
INST(0xCBFB, "SET 7, E",    2,   8, Cpu &cpu) { op_set(7, cpu.registers.e); }
INST(0xCBFC, "SET 7, H",    2,   8, Cpu &cpu) { op_set(7, cpu.registers.h); }
INST(0xCBFD, "SET 7, L",    2,   8, Cpu &cpu) { op_set(7, cpu.registers.l); }
INST(0xCBFE, "SET 7, (HL)", 2,  16, Cpu &cpu) { auto ref = cpu.bus[cpu.registers.hl]; op_set(7, ref); }
INST(0xCBFF, "SET 7, A",    2,   8, Cpu &cpu) { op_set(7, cpu.registers.a); }
