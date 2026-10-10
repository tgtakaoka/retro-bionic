#ifndef __INST_MC68HC16_H__
#define __INST_MC68HC16_H__

#include <stdint.h>

namespace debugger {
namespace mc68hc16 {

struct InstMc68hc16 {
    // Opcodes are words: the CPU16 fetches and executes in words.
    static constexpr uint16_t SWI = 0x3720;
    static constexpr uint16_t NOP = 0x274C;
    static constexpr uint16_t WAI = 0x27F3;
    static constexpr uint16_t LPSTOP = 0x27F1;

    // Exception vectors (CPU16RM Table 9-1): word addresses of the
    // handler's PC in bank 0. The 4-word reset vector is read as program,
    // the others as data.
    static constexpr uint32_t VEC_RESET = 0x0000;
    static constexpr uint32_t VEC_SWI = 0x000C;
    static constexpr uint32_t VEC_IRQ1 = 0x0022;
    static constexpr uint32_t VEC_IRQ7 = 0x002E;

    // Where the debugger parks the CPU and runs its sequences: any even
    // address, since injection never touches memory there.
    static constexpr uint32_t ORG_PARK = 0x0FF00;

    // PK:PC an exception stacks, past the instruction it interrupted
    // (asynchronous) or past the SWI itself (synchronous).
    static constexpr uint32_t IRQ_PC_OFFSET = 6;
    static constexpr uint32_t SWI_PC_OFFSET = 8;

    // Function codes on FC2:FC0.
    static constexpr uint8_t FC_DATA = 5;
    static constexpr uint8_t FC_PROGRAM = 6;
    static constexpr uint8_t FC_CPU = 7;  // an interrupt acknowledge
};

}  // namespace mc68hc16
}  // namespace debugger
#endif /* __INST_MC68HC16_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
