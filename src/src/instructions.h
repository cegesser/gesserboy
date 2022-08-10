#pragma once

#include "cpu.h"
#include <array>
#include <iomanip>
#include <sstream>
#include "operations.h"

template<std::uint16_t OPCODE>
struct Instruction
{
    static constexpr std::uint8_t size = 0;
    static constexpr std::size_t ticks = 0;//TICKS;
    static constexpr const char *mnemonic = "";

    static constexpr bool new_style = false;
    using impl_type = NotImplemented;
    using result_type = typename impl_type::result_type;
};

template<std::int8_t SIZE, std::uint8_t TICKS, typename Impl>
struct Inst {

    static_assert(SIZE == Impl::size);
    static constexpr std::uint8_t size = SIZE;
    static constexpr std::size_t ticks = TICKS;

    using impl_type = Impl;
    using result_type = typename Impl::result_type;

    static constexpr bool new_style = true;
};



template<typename Inst>
inline constexpr std::uint16_t opcode_v=0;

template<std::uint16_t OpCode>
inline constexpr std::uint16_t opcode_v<Instruction<OpCode>> = OpCode;



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
template<> struct Instruction<0x17> : Inst<1,  4, RLA> {};
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
template<> struct Instruction<0x27> : Inst<1,  4, DAA> {};
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
template<> struct Instruction<0x37> : Inst<1,  4, SCF> {};
template<> struct Instruction<0x38> : Inst<2,  0, JR<FlagC,r8>> {};
template<> struct Instruction<0x39> : Inst<1,  8, ADD<HL,SP>> {};
template<> struct Instruction<0x3A> : Inst<1,  8, LD<A, At<HL,'-'>>> {};
template<> struct Instruction<0x3B> : Inst<1,  8, DEC<SP>> {};
template<> struct Instruction<0x3C> : Inst<1,  4, INC<A>> {};
template<> struct Instruction<0x3D> : Inst<1,  4, DEC<A>> {};
template<> struct Instruction<0x3E> : Inst<2,  8, LD<A,d8>> {};
template<> struct Instruction<0x3F> : Inst<1,  4, CCF> {};

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
template<> struct Instruction<0x76> : Inst<1, 4, HALT> {};
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
template<> struct Instruction<0x88> : Inst<1, 4, ADC_A<B>> {};
template<> struct Instruction<0x89> : Inst<1, 4, ADC_A<C>> {};
template<> struct Instruction<0x8A> : Inst<1, 4, ADC_A<D>> {};
template<> struct Instruction<0x8B> : Inst<1, 4, ADC_A<E>> {};
template<> struct Instruction<0x8C> : Inst<1, 4, ADC_A<H>> {};
template<> struct Instruction<0x8D> : Inst<1, 4, ADC_A<L>> {};
template<> struct Instruction<0x8E> : Inst<1, 8, ADC_A<At<HL>>> {};
template<> struct Instruction<0x8F> : Inst<1, 4, ADC_A<A>> {};

template<> struct Instruction<0x90> : Inst<1, 4, SUB<B>> {};
template<> struct Instruction<0x91> : Inst<1, 4, SUB<C>> {};
template<> struct Instruction<0x92> : Inst<1, 4, SUB<D>> {};
template<> struct Instruction<0x93> : Inst<1, 4, SUB<E>> {};
template<> struct Instruction<0x94> : Inst<1, 4, SUB<H>> {};
template<> struct Instruction<0x95> : Inst<1, 4, SUB<L>> {};
template<> struct Instruction<0x96> : Inst<1, 8, SUB<At<HL>>> {};
template<> struct Instruction<0x97> : Inst<1, 4, SUB<A>> {};
template<> struct Instruction<0x98> : Inst<1, 4, SBC_A<B>> {};
template<> struct Instruction<0x99> : Inst<1, 4, SBC_A<C>> {};
template<> struct Instruction<0x9A> : Inst<1, 4, SBC_A<D>> {};
template<> struct Instruction<0x9B> : Inst<1, 4, SBC_A<E>> {};
template<> struct Instruction<0x9C> : Inst<1, 4, SBC_A<H>> {};
template<> struct Instruction<0x9D> : Inst<1, 4, SBC_A<L>> {};
template<> struct Instruction<0x9E> : Inst<1, 8, SBC_A<At<HL>>> {};
template<> struct Instruction<0x9F> : Inst<1, 4, SBC_A<A>> {};

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
template<> struct Instruction<0xCE> : Inst<2,  8, ADC_A<d8>> {};
template<> struct Instruction<0xCF> : Inst<1, 16, RST<0x08>> {};

template<> struct Instruction<0xD0> : Inst<1,  0, RET<FlagNotC>> {};
template<> struct Instruction<0xD1> : Inst<1, 12, POP<DE>> {};
template<> struct Instruction<0xD2> : Inst<3,  0, JP<FlagNotC,a16>> {};
template<> struct Instruction<0xD3> : Inst<1,  4, INVALID> {};
template<> struct Instruction<0xD4> : Inst<3,  0, CALL<FlagNotC,a16>> {};
template<> struct Instruction<0xD5> : Inst<1, 16, PUSH<DE>> {};
template<> struct Instruction<0xD6> : Inst<2,  8, SUB<d8>> {};
template<> struct Instruction<0xD7> : Inst<1, 16, RST<0x10>> {};
template<> struct Instruction<0xD8> : Inst<1,  0, RET<FlagC>> {};
template<> struct Instruction<0xD9> : Inst<1, 16, RETI> {};
template<> struct Instruction<0xDA> : Inst<3,  0, JP<FlagC,a16>> {};
template<> struct Instruction<0xDB> : Inst<1,  4, INVALID> {};
template<> struct Instruction<0xDC> : Inst<3,  0, CALL<FlagC,a16>> {};
template<> struct Instruction<0xDD> : Inst<1,  4, INVALID> {};
template<> struct Instruction<0xDE> : Inst<2,  8, SBC_A<d8>> {};
template<> struct Instruction<0xDF> : Inst<1, 16, RST<0x18>> {};

template<> struct Instruction<0xE0> : Inst<2, 12, LD<At<a8>, A>> {};
template<> struct Instruction<0xE1> : Inst<1, 12, POP<HL>> {};
template<> struct Instruction<0xE2> : Inst<1,  8, LD<At<C>,A>> {};
template<> struct Instruction<0xE3> : Inst<1,  4, INVALID> {};
template<> struct Instruction<0xE4> : Inst<1,  4, INVALID> {};
template<> struct Instruction<0xE5> : Inst<1, 16, PUSH<HL>> {};
template<> struct Instruction<0xE6> : Inst<2,  8, AND<d8>> {};
template<> struct Instruction<0xE7> : Inst<1, 16, RST<0x20>> {};
template<> struct Instruction<0xE8> : Inst<2, 16, ADD<SP,r8>> {};
template<> struct Instruction<0xE9> : Inst<1,  4, JP<Always,HL>> {};
template<> struct Instruction<0xEA> : Inst<3, 12, LD<At<a16>, A>> {};
template<> struct Instruction<0xEB> : Inst<1,  4, INVALID> {};
template<> struct Instruction<0xEC> : Inst<1,  4, INVALID> {};
template<> struct Instruction<0xED> : Inst<1,  4, INVALID> {};
template<> struct Instruction<0xEE> : Inst<2,  8, XOR<d8>> {};
template<> struct Instruction<0xEF> : Inst<1, 16, RST<0x28>> {};

template<> struct Instruction<0xF0> : Inst<2, 12, LD<A, At<a8>>> {};
template<> struct Instruction<0xF1> : Inst<1, 12, POP<AF>> {};
template<> struct Instruction<0xF2> : Inst<1,  8, LD<A, At<C>>> {};
template<> struct Instruction<0xF3> : Inst<1,  4, DI> {};
template<> struct Instruction<0xF4> : Inst<1,  4, INVALID> {};
template<> struct Instruction<0xF5> : Inst<1, 16, PUSH<AF>> {};
template<> struct Instruction<0xF6> : Inst<2,  8, OR<d8>> {};
template<> struct Instruction<0xF7> : Inst<1, 16, RST<0x30>> {};
template<> struct Instruction<0xF8> : Inst<2, 12, LD<HL,SP_r8>> {};//0 0 H C
template<> struct Instruction<0xF9> : Inst<1,  8, LD<SP, HL>> {};
template<> struct Instruction<0xFA> : Inst<3, 16, LD<A,At<a16>>> {};
template<> struct Instruction<0xFB> : Inst<1,  4, EI> {};
template<> struct Instruction<0xFC> : Inst<1,  4, INVALID> {};
template<> struct Instruction<0xFD> : Inst<1,  4, INVALID> {};
template<> struct Instruction<0xFE> : Inst<2,  8, CP<d8>> {};
template<> struct Instruction<0xFF> : Inst<1, 16, RST<0x38>> {};

template<> struct Instruction<0xCB00> : Inst<2,  8, RLC<B>> {};
template<> struct Instruction<0xCB01> : Inst<2,  8, RLC<C>> {};
template<> struct Instruction<0xCB02> : Inst<2,  8, RLC<D>> {};
template<> struct Instruction<0xCB03> : Inst<2,  8, RLC<E>> {};
template<> struct Instruction<0xCB04> : Inst<2,  8, RLC<H>> {};
template<> struct Instruction<0xCB05> : Inst<2,  8, RLC<L>> {};
template<> struct Instruction<0xCB06> : Inst<2, 16, RLC<At<HL>>> {};
template<> struct Instruction<0xCB07> : Inst<2,  8, RLC<A>> {};
template<> struct Instruction<0xCB08> : Inst<2,  8, RRC<B>> {};
template<> struct Instruction<0xCB09> : Inst<2,  8, RRC<C>> {};
template<> struct Instruction<0xCB0A> : Inst<2,  8, RRC<D>> {};
template<> struct Instruction<0xCB0B> : Inst<2,  8, RRC<E>> {};
template<> struct Instruction<0xCB0C> : Inst<2,  8, RRC<H>> {};
template<> struct Instruction<0xCB0D> : Inst<2,  8, RRC<L>> {};
template<> struct Instruction<0xCB0E> : Inst<2, 16, RRC<At<HL>>> {};
template<> struct Instruction<0xCB0F> : Inst<2,  8, RRC<A>> {};

template<> struct Instruction<0xCB10> : Inst<2,  8, RL<B>> {};
template<> struct Instruction<0xCB11> : Inst<2,  8, RL<C>> {};
template<> struct Instruction<0xCB12> : Inst<2,  8, RL<D>> {};
template<> struct Instruction<0xCB13> : Inst<2,  8, RL<E>> {};
template<> struct Instruction<0xCB14> : Inst<2,  8, RL<H>> {};
template<> struct Instruction<0xCB15> : Inst<2,  8, RL<L>> {};
template<> struct Instruction<0xCB16> : Inst<2, 16, RL<At<HL>>> {};
template<> struct Instruction<0xCB17> : Inst<2,  8, RL<A>> {};
template<> struct Instruction<0xCB18> : Inst<2,  8, RR<B>> {};
template<> struct Instruction<0xCB19> : Inst<2,  8, RR<C>> {};
template<> struct Instruction<0xCB1A> : Inst<2,  8, RR<D>> {};
template<> struct Instruction<0xCB1B> : Inst<2,  8, RR<E>> {};
template<> struct Instruction<0xCB1C> : Inst<2,  8, RR<H>> {};
template<> struct Instruction<0xCB1D> : Inst<2,  8, RR<L>> {};
template<> struct Instruction<0xCB1E> : Inst<2, 16, RR<At<HL>>> {};
template<> struct Instruction<0xCB1F> : Inst<2,  8, RR<A>> {};

template<> struct Instruction<0xCB20> : Inst<2,  8, SLA<B>> {};
template<> struct Instruction<0xCB21> : Inst<2,  8, SLA<C>> {};
template<> struct Instruction<0xCB22> : Inst<2,  8, SLA<D>> {};
template<> struct Instruction<0xCB23> : Inst<2,  8, SLA<E>> {};
template<> struct Instruction<0xCB24> : Inst<2,  8, SLA<H>> {};
template<> struct Instruction<0xCB25> : Inst<2,  8, SLA<L>> {};
template<> struct Instruction<0xCB26> : Inst<2, 16, SLA<At<HL>>> {};
template<> struct Instruction<0xCB27> : Inst<2,  8, SLA<A>> {};
template<> struct Instruction<0xCB28> : Inst<2,  8, SRA<B>> {};
template<> struct Instruction<0xCB29> : Inst<2,  8, SRA<C>> {};
template<> struct Instruction<0xCB2A> : Inst<2,  8, SRA<D>> {};
template<> struct Instruction<0xCB2B> : Inst<2,  8, SRA<E>> {};
template<> struct Instruction<0xCB2C> : Inst<2,  8, SRA<H>> {};
template<> struct Instruction<0xCB2D> : Inst<2,  8, SRA<L>> {};
template<> struct Instruction<0xCB2E> : Inst<2, 16, SRA<At<HL>>> {};
template<> struct Instruction<0xCB2F> : Inst<2,  8, SRA<A>> {};

template<> struct Instruction<0xCB30> : Inst<2,  8, SWAP<B>> {};
template<> struct Instruction<0xCB31> : Inst<2,  8, SWAP<C>> {};
template<> struct Instruction<0xCB32> : Inst<2,  8, SWAP<D>> {};
template<> struct Instruction<0xCB33> : Inst<2,  8, SWAP<E>> {};
template<> struct Instruction<0xCB34> : Inst<2,  8, SWAP<H>> {};
template<> struct Instruction<0xCB35> : Inst<2,  8, SWAP<L>> {};
template<> struct Instruction<0xCB36> : Inst<2, 16, SWAP<At<HL>>> {};
template<> struct Instruction<0xCB37> : Inst<2,  8, SWAP<A>> {};
template<> struct Instruction<0xCB38> : Inst<2,  8, SRL<B>> {};
template<> struct Instruction<0xCB39> : Inst<2,  8, SRL<C>> {};
template<> struct Instruction<0xCB3A> : Inst<2,  8, SRL<D>> {};
template<> struct Instruction<0xCB3B> : Inst<2,  8, SRL<E>> {};
template<> struct Instruction<0xCB3C> : Inst<2,  8, SRL<H>> {};
template<> struct Instruction<0xCB3D> : Inst<2,  8, SRL<L>> {};
template<> struct Instruction<0xCB3E> : Inst<2, 16, SRL<At<HL>>> {};
template<> struct Instruction<0xCB3F> : Inst<2,  8, SRL<A>> {};

template<> struct Instruction<0xCB40> : Inst<2,  8, BIT<0, B>> {};
template<> struct Instruction<0xCB41> : Inst<2,  8, BIT<0, C>> {};
template<> struct Instruction<0xCB42> : Inst<2,  8, BIT<0, D>> {};
template<> struct Instruction<0xCB43> : Inst<2,  8, BIT<0, E>> {};
template<> struct Instruction<0xCB44> : Inst<2,  8, BIT<0, H>> {};
template<> struct Instruction<0xCB45> : Inst<2,  8, BIT<0, L>> {};
template<> struct Instruction<0xCB46> : Inst<2, 12, BIT<0, At<HL>>> {};
template<> struct Instruction<0xCB47> : Inst<2,  8, BIT<0, A>> {};
template<> struct Instruction<0xCB48> : Inst<2,  8, BIT<1, B>> {};
template<> struct Instruction<0xCB49> : Inst<2,  8, BIT<1, C>> {};
template<> struct Instruction<0xCB4A> : Inst<2,  8, BIT<1, D>> {};
template<> struct Instruction<0xCB4B> : Inst<2,  8, BIT<1, E>> {};
template<> struct Instruction<0xCB4C> : Inst<2,  8, BIT<1, H>> {};
template<> struct Instruction<0xCB4D> : Inst<2,  8, BIT<1, L>> {};
template<> struct Instruction<0xCB4E> : Inst<2, 12, BIT<1, At<HL>>> {};
template<> struct Instruction<0xCB4F> : Inst<2,  8, BIT<1, A>> {};

template<> struct Instruction<0xCB50> : Inst<2,  8, BIT<2, B>> {};
template<> struct Instruction<0xCB51> : Inst<2,  8, BIT<2, C>> {};
template<> struct Instruction<0xCB52> : Inst<2,  8, BIT<2, D>> {};
template<> struct Instruction<0xCB53> : Inst<2,  8, BIT<2, E>> {};
template<> struct Instruction<0xCB54> : Inst<2,  8, BIT<2, H>> {};
template<> struct Instruction<0xCB55> : Inst<2,  8, BIT<2, L>> {};
template<> struct Instruction<0xCB56> : Inst<2, 12, BIT<2, At<HL>>> {};
template<> struct Instruction<0xCB57> : Inst<2,  8, BIT<2, A>> {};
template<> struct Instruction<0xCB58> : Inst<2,  8, BIT<3, B>> {};
template<> struct Instruction<0xCB59> : Inst<2,  8, BIT<3, C>> {};
template<> struct Instruction<0xCB5A> : Inst<2,  8, BIT<3, D>> {};
template<> struct Instruction<0xCB5B> : Inst<2,  8, BIT<3, E>> {};
template<> struct Instruction<0xCB5C> : Inst<2,  8, BIT<3, H>> {};
template<> struct Instruction<0xCB5D> : Inst<2,  8, BIT<3, L>> {};
template<> struct Instruction<0xCB5E> : Inst<2, 12, BIT<3, At<HL>>> {};
template<> struct Instruction<0xCB5F> : Inst<2,  8, BIT<3, A>> {};

template<> struct Instruction<0xCB60> : Inst<2,  8, BIT<4, B>> {};
template<> struct Instruction<0xCB61> : Inst<2,  8, BIT<4, C>> {};
template<> struct Instruction<0xCB62> : Inst<2,  8, BIT<4, D>> {};
template<> struct Instruction<0xCB63> : Inst<2,  8, BIT<4, E>> {};
template<> struct Instruction<0xCB64> : Inst<2,  8, BIT<4, H>> {};
template<> struct Instruction<0xCB65> : Inst<2,  8, BIT<4, L>> {};
template<> struct Instruction<0xCB66> : Inst<2, 12, BIT<4, At<HL>>> {};
template<> struct Instruction<0xCB67> : Inst<2,  8, BIT<4, A>> {};
template<> struct Instruction<0xCB68> : Inst<2,  8, BIT<5, B>> {};
template<> struct Instruction<0xCB69> : Inst<2,  8, BIT<5, C>> {};
template<> struct Instruction<0xCB6A> : Inst<2,  8, BIT<5, D>> {};
template<> struct Instruction<0xCB6B> : Inst<2,  8, BIT<5, E>> {};
template<> struct Instruction<0xCB6C> : Inst<2,  8, BIT<5, H>> {};
template<> struct Instruction<0xCB6D> : Inst<2,  8, BIT<5, L>> {};
template<> struct Instruction<0xCB6E> : Inst<2, 12, BIT<5, At<HL>>> {};
template<> struct Instruction<0xCB6F> : Inst<2,  8, BIT<5, A>> {};

template<> struct Instruction<0xCB70> : Inst<2,  8, BIT<6, B>> {};
template<> struct Instruction<0xCB71> : Inst<2,  8, BIT<6, C>> {};
template<> struct Instruction<0xCB72> : Inst<2,  8, BIT<6, D>> {};
template<> struct Instruction<0xCB73> : Inst<2,  8, BIT<6, E>> {};
template<> struct Instruction<0xCB74> : Inst<2,  8, BIT<6, H>> {};
template<> struct Instruction<0xCB75> : Inst<2,  8, BIT<6, L>> {};
template<> struct Instruction<0xCB76> : Inst<2, 12, BIT<6, At<HL>>> {};
template<> struct Instruction<0xCB77> : Inst<2,  8, BIT<6, A>> {};
template<> struct Instruction<0xCB78> : Inst<2,  8, BIT<7, B>> {};
template<> struct Instruction<0xCB79> : Inst<2,  8, BIT<7, C>> {};
template<> struct Instruction<0xCB7A> : Inst<2,  8, BIT<7, D>> {};
template<> struct Instruction<0xCB7B> : Inst<2,  8, BIT<7, E>> {};
template<> struct Instruction<0xCB7C> : Inst<2,  8, BIT<7, H>> {};
template<> struct Instruction<0xCB7D> : Inst<2,  8, BIT<7, L>> {};
template<> struct Instruction<0xCB7E> : Inst<2, 12, BIT<7, At<HL>>> {};
template<> struct Instruction<0xCB7F> : Inst<2,  8, BIT<7, A>> {};

template<> struct Instruction<0xCB80> : Inst<2,  8, RES<0, B>> {};
template<> struct Instruction<0xCB81> : Inst<2,  8, RES<0, C>> {};
template<> struct Instruction<0xCB82> : Inst<2,  8, RES<0, D>> {};
template<> struct Instruction<0xCB83> : Inst<2,  8, RES<0, E>> {};
template<> struct Instruction<0xCB84> : Inst<2,  8, RES<0, H>> {};
template<> struct Instruction<0xCB85> : Inst<2,  8, RES<0, L>> {};
template<> struct Instruction<0xCB86> : Inst<2, 16, RES<0, At<HL>>> {};
template<> struct Instruction<0xCB87> : Inst<2,  8, RES<0, A>> {};
template<> struct Instruction<0xCB88> : Inst<2,  8, RES<1, B>> {};
template<> struct Instruction<0xCB89> : Inst<2,  8, RES<1, C>> {};
template<> struct Instruction<0xCB8A> : Inst<2,  8, RES<1, D>> {};
template<> struct Instruction<0xCB8B> : Inst<2,  8, RES<1, E>> {};
template<> struct Instruction<0xCB8C> : Inst<2,  8, RES<1, H>> {};
template<> struct Instruction<0xCB8D> : Inst<2,  8, RES<1, L>> {};
template<> struct Instruction<0xCB8E> : Inst<2, 16, RES<1, At<HL>>> {};
template<> struct Instruction<0xCB8F> : Inst<2,  8, RES<1, A>> {};

template<> struct Instruction<0xCB90> : Inst<2,  8, RES<2, B>> {};
template<> struct Instruction<0xCB91> : Inst<2,  8, RES<2, C>> {};
template<> struct Instruction<0xCB92> : Inst<2,  8, RES<2, D>> {};
template<> struct Instruction<0xCB93> : Inst<2,  8, RES<2, E>> {};
template<> struct Instruction<0xCB94> : Inst<2,  8, RES<2, H>> {};
template<> struct Instruction<0xCB95> : Inst<2,  8, RES<2, L>> {};
template<> struct Instruction<0xCB96> : Inst<2, 16, RES<2, At<HL>>> {};
template<> struct Instruction<0xCB97> : Inst<2,  8, RES<2, A>> {};
template<> struct Instruction<0xCB98> : Inst<2,  8, RES<3, B>> {};
template<> struct Instruction<0xCB99> : Inst<2,  8, RES<3, C>> {};
template<> struct Instruction<0xCB9A> : Inst<2,  8, RES<3, D>> {};
template<> struct Instruction<0xCB9B> : Inst<2,  8, RES<3, E>> {};
template<> struct Instruction<0xCB9C> : Inst<2,  8, RES<3, H>> {};
template<> struct Instruction<0xCB9D> : Inst<2,  8, RES<3, L>> {};
template<> struct Instruction<0xCB9E> : Inst<2, 16, RES<3, At<HL>>> {};
template<> struct Instruction<0xCB9F> : Inst<2,  8, RES<3, A>> {};

template<> struct Instruction<0xCBA0> : Inst<2,  8, RES<4, B>> {};
template<> struct Instruction<0xCBA1> : Inst<2,  8, RES<4, C>> {};
template<> struct Instruction<0xCBA2> : Inst<2,  8, RES<4, D>> {};
template<> struct Instruction<0xCBA3> : Inst<2,  8, RES<4, E>> {};
template<> struct Instruction<0xCBA4> : Inst<2,  8, RES<4, H>> {};
template<> struct Instruction<0xCBA5> : Inst<2,  8, RES<4, L>> {};
template<> struct Instruction<0xCBA6> : Inst<2, 16, RES<4, At<HL>>> {};
template<> struct Instruction<0xCBA7> : Inst<2,  8, RES<4, A>> {};
template<> struct Instruction<0xCBA8> : Inst<2,  8, RES<5, B>> {};
template<> struct Instruction<0xCBA9> : Inst<2,  8, RES<5, C>> {};
template<> struct Instruction<0xCBAA> : Inst<2,  8, RES<5, D>> {};
template<> struct Instruction<0xCBAB> : Inst<2,  8, RES<5, E>> {};
template<> struct Instruction<0xCBAC> : Inst<2,  8, RES<5, H>> {};
template<> struct Instruction<0xCBAD> : Inst<2,  8, RES<5, L>> {};
template<> struct Instruction<0xCBAE> : Inst<2, 16, RES<5, At<HL>>> {};
template<> struct Instruction<0xCBAF> : Inst<2,  8, RES<5, A>> {};

template<> struct Instruction<0xCBB0> : Inst<2,  8, RES<6, B>> {};
template<> struct Instruction<0xCBB1> : Inst<2,  8, RES<6, C>> {};
template<> struct Instruction<0xCBB2> : Inst<2,  8, RES<6, D>> {};
template<> struct Instruction<0xCBB3> : Inst<2,  8, RES<6, E>> {};
template<> struct Instruction<0xCBB4> : Inst<2,  8, RES<6, H>> {};
template<> struct Instruction<0xCBB5> : Inst<2,  8, RES<6, L>> {};
template<> struct Instruction<0xCBB6> : Inst<2, 16, RES<6, At<HL>>> {};
template<> struct Instruction<0xCBB7> : Inst<2,  8, RES<6, A>> {};
template<> struct Instruction<0xCBB8> : Inst<2,  8, RES<7, B>> {};
template<> struct Instruction<0xCBB9> : Inst<2,  8, RES<7, C>> {};
template<> struct Instruction<0xCBBA> : Inst<2,  8, RES<7, D>> {};
template<> struct Instruction<0xCBBB> : Inst<2,  8, RES<7, E>> {};
template<> struct Instruction<0xCBBC> : Inst<2,  8, RES<7, H>> {};
template<> struct Instruction<0xCBBD> : Inst<2,  8, RES<7, L>> {};
template<> struct Instruction<0xCBBE> : Inst<2, 16, RES<7, At<HL>>> {};
template<> struct Instruction<0xCBBF> : Inst<2,  8, RES<7, A>> {};

template<> struct Instruction<0xCBC0> : Inst<2,  8, SET<0, B>> {};
template<> struct Instruction<0xCBC1> : Inst<2,  8, SET<0, C>> {};
template<> struct Instruction<0xCBC2> : Inst<2,  8, SET<0, D>> {};
template<> struct Instruction<0xCBC3> : Inst<2,  8, SET<0, E>> {};
template<> struct Instruction<0xCBC4> : Inst<2,  8, SET<0, H>> {};
template<> struct Instruction<0xCBC5> : Inst<2,  8, SET<0, L>> {};
template<> struct Instruction<0xCBC6> : Inst<2, 16, SET<0, At<HL>>> {};
template<> struct Instruction<0xCBC7> : Inst<2,  8, SET<0, A>> {};
template<> struct Instruction<0xCBC8> : Inst<2,  8, SET<1, B>> {};
template<> struct Instruction<0xCBC9> : Inst<2,  8, SET<1, C>> {};
template<> struct Instruction<0xCBCA> : Inst<2,  8, SET<1, D>> {};
template<> struct Instruction<0xCBCB> : Inst<2,  8, SET<1, E>> {};
template<> struct Instruction<0xCBCC> : Inst<2,  8, SET<1, H>> {};
template<> struct Instruction<0xCBCD> : Inst<2,  8, SET<1, L>> {};
template<> struct Instruction<0xCBCE> : Inst<2, 16, SET<1, At<HL>>> {};
template<> struct Instruction<0xCBCF> : Inst<2,  8, SET<1, A>> {};

template<> struct Instruction<0xCBD0> : Inst<2,  8, SET<2, B>> {};
template<> struct Instruction<0xCBD1> : Inst<2,  8, SET<2, C>> {};
template<> struct Instruction<0xCBD2> : Inst<2,  8, SET<2, D>> {};
template<> struct Instruction<0xCBD3> : Inst<2,  8, SET<2, E>> {};
template<> struct Instruction<0xCBD4> : Inst<2,  8, SET<2, H>> {};
template<> struct Instruction<0xCBD5> : Inst<2,  8, SET<2, L>> {};
template<> struct Instruction<0xCBD6> : Inst<2, 16, SET<2, At<HL>>> {};
template<> struct Instruction<0xCBD7> : Inst<2,  8, SET<2, A>> {};
template<> struct Instruction<0xCBD8> : Inst<2,  8, SET<3, B>> {};
template<> struct Instruction<0xCBD9> : Inst<2,  8, SET<3, C>> {};
template<> struct Instruction<0xCBDA> : Inst<2,  8, SET<3, D>> {};
template<> struct Instruction<0xCBDB> : Inst<2,  8, SET<3, E>> {};
template<> struct Instruction<0xCBDC> : Inst<2,  8, SET<3, H>> {};
template<> struct Instruction<0xCBDD> : Inst<2,  8, SET<3, L>> {};
template<> struct Instruction<0xCBDE> : Inst<2, 16, SET<3, At<HL>>> {};
template<> struct Instruction<0xCBDF> : Inst<2,  8, SET<3, A>> {};

template<> struct Instruction<0xCBE0> : Inst<2,  8, SET<4, B>> {};
template<> struct Instruction<0xCBE1> : Inst<2,  8, SET<4, C>> {};
template<> struct Instruction<0xCBE2> : Inst<2,  8, SET<4, D>> {};
template<> struct Instruction<0xCBE3> : Inst<2,  8, SET<4, E>> {};
template<> struct Instruction<0xCBE4> : Inst<2,  8, SET<4, H>> {};
template<> struct Instruction<0xCBE5> : Inst<2,  8, SET<4, L>> {};
template<> struct Instruction<0xCBE6> : Inst<2, 16, SET<4, At<HL>>> {};
template<> struct Instruction<0xCBE7> : Inst<2,  8, SET<4, A>> {};
template<> struct Instruction<0xCBE8> : Inst<2,  8, SET<5, B>> {};
template<> struct Instruction<0xCBE9> : Inst<2,  8, SET<5, C>> {};
template<> struct Instruction<0xCBEA> : Inst<2,  8, SET<5, D>> {};
template<> struct Instruction<0xCBEB> : Inst<2,  8, SET<5, E>> {};
template<> struct Instruction<0xCBEC> : Inst<2,  8, SET<5, H>> {};
template<> struct Instruction<0xCBED> : Inst<2,  8, SET<5, L>> {};
template<> struct Instruction<0xCBEE> : Inst<2, 16, SET<5, At<HL>>> {};
template<> struct Instruction<0xCBEF> : Inst<2,  8, SET<5, A>> {};

template<> struct Instruction<0xCBF0> : Inst<2,  8, SET<6, B>> {};
template<> struct Instruction<0xCBF1> : Inst<2,  8, SET<6, C>> {};
template<> struct Instruction<0xCBF2> : Inst<2,  8, SET<6, D>> {};
template<> struct Instruction<0xCBF3> : Inst<2,  8, SET<6, E>> {};
template<> struct Instruction<0xCBF4> : Inst<2,  8, SET<6, H>> {};
template<> struct Instruction<0xCBF5> : Inst<2,  8, SET<6, L>> {};
template<> struct Instruction<0xCBF6> : Inst<2, 16, SET<6, At<HL>>> {};
template<> struct Instruction<0xCBF7> : Inst<2,  8, SET<6, A>> {};
template<> struct Instruction<0xCBF8> : Inst<2,  8, SET<7, B>> {};
template<> struct Instruction<0xCBF9> : Inst<2,  8, SET<7, C>> {};
template<> struct Instruction<0xCBFA> : Inst<2,  8, SET<7, D>> {};
template<> struct Instruction<0xCBFB> : Inst<2,  8, SET<7, E>> {};
template<> struct Instruction<0xCBFC> : Inst<2,  8, SET<7, H>> {};
template<> struct Instruction<0xCBFD> : Inst<2,  8, SET<7, L>> {};
template<> struct Instruction<0xCBFE> : Inst<2, 16, SET<7, At<HL>>> {};
template<> struct Instruction<0xCBFF> : Inst<2,  8, SET<7, A>> {};