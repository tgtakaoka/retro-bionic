#ifndef __INST_Z8000_H__
#define __INST_Z8000_H__

#include <stdint.h>

namespace debugger {
namespace z8000 {

struct InstZ8000 {
    static constexpr uint16_t HALT = 0x7A00;
    static constexpr uint16_t SC = 0x7F00;  // SC #n: 7Fnn
    // A breakpoint, or a program's exit to the debugger.
    static constexpr uint16_t SC_BREAK = SC | 0xFF;
    static constexpr uint16_t JR_NEXT = 0xE800;  // JR $+2
    // What sets the FCW: IRET from the stack, LDPS from memory, and
    // LDCTL FCW,Rs (7Ds A) from a register.
    static constexpr uint16_t IRET = 0x7B00;
    static bool ldctlFcw(uint16_t op) { return (op & 0xFF0F) == 0x7D0A; }
    static bool ldps(uint16_t op) {
        return (op & 0xFF0F) == 0x3900 || (op & 0xFF0F) == 0x7900;
    }

    // Reset vector (Section 7.4); the Z8001's PC takes two words.
    static constexpr uint16_t ORG_FCW = 0x0002;
    static constexpr uint16_t ORG_PC = 0x0004;
    static constexpr uint16_t ORG_PC_OFF = 0x0006;
    static constexpr uint16_t FCW_SEG = 0x8000;
    static constexpr uint16_t FCW_SN = 0x4000;
    // The debugger's: system mode, interrupts disabled.
    static constexpr uint16_t SYS_FCW = FCW_SN;
    static constexpr uint16_t SYS_FCW_SEG = FCW_SEG | SYS_FCW;
};

}  // namespace z8000
}  // namespace debugger
#endif /* __INST_Z8000_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
