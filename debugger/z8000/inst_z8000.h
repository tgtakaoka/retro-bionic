#ifndef __INST_Z8000_H__
#define __INST_Z8000_H__

#include <stdint.h>

namespace debugger {
namespace z8000 {

struct InstZ8000 {
    static constexpr uint16_t HALT = 0x7A00;
    static constexpr uint16_t SC = 0x7F00;  // SC #n: 7Fnn
    // A breakpoint, and the samples' way back to the debugger.
    static constexpr uint16_t SC_BREAK = SC | 0xFF;
    static constexpr uint16_t SC_EXIT = SC | 0xFE;
    static constexpr uint16_t JR_NEXT = 0xE800;  // JR $+2

    // Where reset reads the FCW and PC (Z8002, Figure 7-2).
    static constexpr uint16_t ORG_FCW = 0x0002;
    static constexpr uint16_t ORG_PC = 0x0004;
    // System mode, interrupts disabled: what the debugger runs with.
    static constexpr uint16_t FCW_SN = 0x4000;
    static constexpr uint16_t SYS_FCW = FCW_SN;
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
