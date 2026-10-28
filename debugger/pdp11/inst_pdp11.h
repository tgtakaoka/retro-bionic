#ifndef __INST_PDP11_H__
#define __INST_PDP11_H__

#include <stdint.h>

namespace debugger {
namespace pdp11 {

struct InstPdp11 {
    static constexpr uint16_t HALT = 0000000;
    // A breakpoint, or a program's exit to the debugger.
    static constexpr uint16_t BPT = 0000003;
    static constexpr uint16_t RTT = 0000006;
    static constexpr uint16_t JMP_ABS = 0000137;      // JMP @#addr
    static constexpr uint16_t MFPS_ABS = 0106737;     // MFPS @#addr
    static constexpr uint16_t MOV_IMM = 0012700;      // MOV #imm, Rn
    static constexpr uint16_t MOV_ABS = 0010037;      // MOV Rn, @#addr
    static constexpr uint16_t movImm(uint8_t reg) { return MOV_IMM | reg; }
    static constexpr uint16_t movAbs(uint8_t reg) {
        return MOV_ABS | (reg << 6);
    }
    static constexpr uint8_t SP = 6;

    // BPT and the T-bit trace trap.
    static constexpr uint16_t VEC_BPT = 0000014;
    static constexpr uint16_t PSW_T = 0000020;
    // The debugger's: priority 7, interrupts masked.
    static constexpr uint16_t PSW_PRI7 = 0000340;
};

}  // namespace pdp11
}  // namespace debugger
#endif /* __INST_PDP11_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
