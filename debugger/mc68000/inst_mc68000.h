#ifndef __INST_MC68000_H__
#define __INST_MC68000_H__

#include <stdint.h>

namespace debugger {
namespace mc68000 {

struct InstMc68000 {
    // Opcodes are words, the even byte first.
    static constexpr uint16_t ILLEGAL = 0x4AFC;
    static constexpr uint16_t NOP = 0x4E71;
    static constexpr uint16_t RTE = 0x4E73;

    // Exception vectors with VBR 0 (M68000UM Table 6-2): long words, the
    // handler's address. The reset vector holds SSP, then PC, and is read
    // as supervisor program; the others as supervisor data.
    static constexpr uint32_t VEC_RESET = 0x000000;
    static constexpr uint32_t VEC_ILLEGAL = 0x000010;
    static constexpr uint32_t VEC_TRACE = 0x000024;
    static constexpr uint32_t vecAddr(uint8_t vecNum) { return vecNum * 4; }

    // The vector number a level-7 acknowledge is answered with: the
    // level-7 autovector, read at 00007CH.
    static constexpr uint8_t VECNUM_HALT = 31;
    // For an acknowledge no device claims.
    static constexpr uint8_t VECNUM_SPURIOUS = 24;

    // Where the debugger parks the CPU and runs its sequences: any even
    // address, since injection never touches memory there.
    static constexpr uint32_t ORG_PARK = 0x0FF000;

    // Status register (M68000PRM 1.3.2).
    static constexpr uint16_t SR_T = 0x8000;
    static constexpr uint16_t SR_S = 0x2000;
    static constexpr uint16_t SR_I = 0x0700;
    // After reset: supervisor, interrupts masked to level 7.
    static constexpr uint16_t SR_RESET = SR_S | SR_I;
};

}  // namespace mc68000
}  // namespace debugger
#endif /* __INST_MC68000_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
