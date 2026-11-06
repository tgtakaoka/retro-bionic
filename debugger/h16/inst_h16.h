#ifndef __INST_H16_H__
#define __INST_H16_H__

#include <stdint.h>

namespace debugger {
namespace h16 {

struct InstH16 {
    static constexpr uint8_t NOP = 0xFF;
    // An unimplemented opcode (H'C0-H'C7), the breakpoint: one byte, and
    // its exception stacks its own address (User's Manual Table 4-16).
    static constexpr uint8_t BREAK = 0xC0;
    // TRAPA #15, the halt a program ends with: it stacks the address
    // past itself, as TRAP does.
    static constexpr uint8_t TRAPA = 0xF2;
    static constexpr uint8_t TRAPA_HALT = 0x0F;
    static constexpr uint32_t HALT_PC_OFFSET = 2;

    // Exception vectors: offsets from EBR (User's Manual Table 4-13). The
    // reset vector is SSP then PC at 000000H, read as program.
    static constexpr uint32_t VEC_RESET = 0x000;
    static constexpr uint32_t VEC_TRACE = 0x024;
    static constexpr uint32_t VEC_BREAK = 0x028;  // H'C0-H'C7
    static constexpr uint32_t VEC_NMI = 0x07C;
    static constexpr uint32_t VEC_HALT = 0x0BC;  // TRAPA #15
    // The vector table spans 256 vectors from EBR.
    static constexpr uint32_t VEC_END = 0x400;

    // SR bits (User's Manual Figure 4-11).
    static constexpr uint16_t SR_T = 0x8000;
    static constexpr uint16_t SR_S = 0x2000;

    // Where the debugger parks the CPU and runs its sequences: injection
    // never touches memory there. Clear of the reset vectors, the on-chip
    // RAM at FFF800H and the on-chip I/O at FFFE00H.
    static constexpr uint32_t ORG_PARK = 0xFFE000;
};

}  // namespace h16
}  // namespace debugger
#endif /* __INST_H16_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
