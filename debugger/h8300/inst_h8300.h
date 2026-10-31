#ifndef __INST_H8300_H__
#define __INST_H8300_H__

#include <stdint.h>

namespace debugger {
namespace h8300 {

struct InstH8300 {
    // Instructions are words, high byte first.
    static constexpr uint8_t NOP = 0x00;
    // JSR @@aa:8: the H8/300 has no trap instruction. A breakpoint is its
    // opcode alone, reading its vector wherever the patched instruction's
    // second byte points. The halt a program ends with is JSR @@H'04:
    // H'0002-H'0005 are reserved vectors (H8/325 Series Hardware Manual
    // Table 4-2), so that vector never belongs to a program.
    static constexpr uint8_t BREAK = 0x5F;
    static constexpr uint8_t HALT_VEC = 0x04;
    static constexpr uint8_t BREAK_LEN = 2;

    // Exception vectors in mode 1 (Table 4-2): one word each, the
    // handler's address.
    static constexpr uint16_t VEC_RESET = 0x0000;
    static constexpr uint16_t VEC_HALT = 0x0004;
    static constexpr uint16_t VEC_NMI = 0x0006;
    static constexpr uint16_t VEC_END = 0x0030;

    // CCR bits (Programming Manual 1.3.2): I, U, H, U, N, Z, V, C.
    static constexpr uint8_t CCR_I = 0x80;

    // Where the debugger parks the CPU and runs its sequences, and its
    // stack until a program sets one: external, and clear of every sample.
    static constexpr uint16_t ORG_PARK = 0xF000;
    static constexpr uint16_t STACK_PARK = 0xF000;

    // The on-chip register field (Figure 2-1): accessed with #AS, #RD and
    // #WR high, so it never shows on the bus. H'FF80-H'FF8F and
    // H'FFA0-H'FFAF between them are external.
    static constexpr bool registerField(uint16_t addr) {
        return (addr >= 0xFF90 && addr < 0xFFA0) || addr >= 0xFFB0;
    }
};

}  // namespace h8300
}  // namespace debugger
#endif /* __INST_H8300_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
