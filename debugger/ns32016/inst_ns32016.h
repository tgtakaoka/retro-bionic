#ifndef __INST_NS32016_H__
#define __INST_NS32016_H__

#include <stdint.h>

namespace debugger {
namespace ns32016 {

struct InstNs32016 {
    static constexpr uint8_t BPT = 0xF2;
    static constexpr uint8_t NOP = 0xA2;
    static constexpr uint8_t WAIT = 0xB2;
    static constexpr uint8_t BR = 0xEA;  // BR disp, from its own first byte
    static constexpr uint8_t RETT = 0x42;

    static constexpr uint16_t PSR_T = 0x0002;
    static constexpr uint16_t PSR_P = 0x0400;

    // The debugger's trap handler: a trap's program base is answered so
    // its handler lands here, and the CPU parks on the handler's fetch.
    static constexpr uint32_t PARK_PC = 0xFFF000;
    // The trap frame: return PC, MOD and PSR, below the stack pointer.
    static constexpr uint8_t FRAME_BYTES = 8;
    static constexpr uint32_t ADDR_MASK = 0xFFFFFF;
};

}  // namespace ns32016
}  // namespace debugger
#endif /* __INST_NS32016_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
