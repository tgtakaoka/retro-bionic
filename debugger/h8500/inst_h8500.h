#ifndef __INST_H8500_H__
#define __INST_H8500_H__

#include <stdint.h>

namespace debugger {
namespace h8500 {

struct InstH8500 {
    static constexpr uint8_t NOP = 0x00;
    // H'0B is undefined, so it raises the invalid instruction exception
    // (Programming Manual Table 2-10 (a)): one byte, the breakpoint.
    static constexpr uint8_t BREAK = 0x0B;
    // TRAPA #15, the halt a program ends with: it stacks the address past
    // itself, as every trap does.
    static constexpr uint8_t TRAPA = 0x08;
    static constexpr uint8_t TRAPA_HALT = 0x1F;
    static constexpr uint32_t HALT_PC_OFFSET = 2;

    // Exception vectors in maximum mode, page 0 (Programming Manual Table
    // 3-2): a word holding CP in its low byte, then PC.
    static constexpr uint32_t VEC_RESET = 0x000;
    static constexpr uint32_t VEC_BREAK = 0x008;  // invalid instruction
    static constexpr uint32_t VEC_TRACE = 0x024;
    static constexpr uint32_t VEC_NMI = 0x02C;
    static constexpr uint32_t VEC_HALT = 0x07C;  // TRAPA #15
    static constexpr uint32_t VEC_END = 0x140;

    // SR bits (Programming Manual Figure 1-1): T, I2-I0, then CCR.
    static constexpr uint16_t SR_T = 0x8000;
    static constexpr uint16_t SR_I = 0x0700;

    // Where the debugger parks the CPU and runs its sequences, and its
    // stack until a program sets one: page H'0F, external on both chips
    // and clear of every sample, which stay in page 0.
    static constexpr uint32_t ORG_PARK = 0x0F0000;
    static constexpr uint32_t STACK_PARK = 0x0FFF00;
};

}  // namespace h8500
}  // namespace debugger
#endif /* __INST_H8500_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
