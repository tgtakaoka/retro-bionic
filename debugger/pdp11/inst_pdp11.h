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

    // FP11, in double precision after SETD.
    static constexpr uint16_t SETD = 0170011;
    static constexpr uint16_t LDFPS_ABS = 0170137;  // LDFPS @#addr
    static constexpr uint16_t STFPS_ABS = 0170237;  // STFPS @#addr
    static constexpr uint16_t LDD_ABS = 0172437;    // LDD @#addr, ACn
    static constexpr uint16_t STD_ABS = 0174037;    // STD ACn, @#addr
    static constexpr uint16_t LDD_AC4 = 0172404;    // LDD AC4, AC0
    static constexpr uint16_t STD_AC4 = 0174004;    // STD AC0, AC4
    static constexpr uint16_t lddAbs(uint8_t ac) { return LDD_ABS | (ac << 6); }
    static constexpr uint16_t stdAbs(uint8_t ac) { return STD_ABS | (ac << 6); }

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
