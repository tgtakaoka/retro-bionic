#ifndef __INST_MOS6502_H__
#define __INST_MOS6502_H__

#include <stdint.h>

namespace debugger {
namespace mos6502 {

struct InstMos6502 {
    static constexpr uint8_t BRK = 0x00;
    static constexpr uint8_t BBR0 = 0x0F;
    static constexpr uint8_t CLC = 0x18;
    static constexpr uint8_t SEC = 0x38;
    static constexpr uint8_t JMP = 0x4C;
    static constexpr uint8_t BRA = 0x80;
    static constexpr uint8_t NOP = 0xEA;
    static bool isStop(uint8_t inst);

    static constexpr uint16_t VECTOR_IRQ = 0xFFFE;
    static constexpr uint16_t VECTOR_RESET = 0xFFFC;
    static constexpr uint16_t VECTOR_NMI = 0xFFFA;

    // 65816 emulation mode
    static constexpr uint16_t VECTOR_ABORT = 0xFFF8;
    static constexpr uint16_t VECTOR_COP = 0xFFF4;
    // 65816 native mode
    static constexpr uint16_t W65816_IRQ = 0xFFEE;
    static constexpr uint16_t W65816_NMI = 0xFFEA;
    static constexpr uint16_t W65816_ABORT = 0xFFE8;
    static constexpr uint16_t W65816_BRK = 0xFFE6;
    static constexpr uint16_t W65816_COP = 0xFFE4;

private:
    static constexpr uint8_t WAI = 0xCB;
    static constexpr uint8_t STP = 0xDB;
};
}  // namespace mos6502
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
