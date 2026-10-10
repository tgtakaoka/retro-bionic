#ifndef __INST_MC68HC12_H__
#define __INST_MC68HC12_H__

#include <stdint.h>

namespace debugger {
namespace mc68hc12 {

// The CPU12 opcodes and vectors the debugger injects and watches.
struct InstMc68hc12 {
    static constexpr uint8_t BRA = 0x20;
    static constexpr uint8_t BRA_HERE = 0xFE;  // BRA *
    static constexpr uint8_t NOP = 0xA7;
    static constexpr uint8_t SWI = 0x3F;
    static constexpr uint8_t RTI = 0x0B;
    static constexpr uint8_t PSHA = 0x36;
    static constexpr uint8_t PSHD = 0x3B;
    static constexpr uint8_t LDAA_IMM = 0x86;
    static constexpr uint8_t LDAA_EXT = 0xB6;
    static constexpr uint8_t LDD_IMM = 0xCC;
    static constexpr uint8_t LDS_IMM = 0xCF;
    static constexpr uint8_t STAA_EXT = 0x7A;

    static constexpr uint16_t VEC_IRQ = 0xFFF2;
    static constexpr uint16_t VEC_XIRQ = 0xFFF4;
    static constexpr uint16_t VEC_SWI = 0xFFF6;
    static constexpr uint16_t VEC_RESET = 0xFFFE;
};

}  // namespace mc68hc12
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
