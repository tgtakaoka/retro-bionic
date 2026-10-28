#ifndef __INST_I8086_H__
#define __INST_I8086_H__

#include <stdint.h>

namespace debugger {
namespace i8086 {

struct InstI8086 {
    static constexpr uint8_t INT3 = 0xCC;  // a breakpoint
    static constexpr uint8_t HLT = 0xF4;
    static constexpr uint8_t IRET = 0xCF;
    static constexpr uint8_t JMPF = 0xEA;  // JMP ptr16:16
    static constexpr uint8_t PUSHF = 0x9C;

    // Reset starts at FFFF:0000 (8086 datasheet, Table 2).
    static constexpr uint16_t RESET_CS = 0xFFFF;
    static constexpr uint16_t RESET_IP = 0x0000;

    // The vectors the debugger serves: single step, NMI and INT 3.
    static constexpr uint8_t VEC_STEP = 1;
    static constexpr uint8_t VEC_NMI = 2;
    static constexpr uint8_t VEC_BREAK = 3;
    static constexpr uint32_t VECTORS = 4 * VEC_STEP;
    static constexpr uint32_t VECTORS_END = 4 * (VEC_BREAK + 1);

    // Where the debugger's trap handler parks the CPU: its vectors point
    // here and every injected sequence starts here after a trap.
    static constexpr uint16_t PARK_CS = 0x0000;
    static constexpr uint16_t PARK_IP = 0x0400;

    // FLAGS
    static constexpr uint16_t FLAGS_TF = 0x0100;

    static constexpr uint32_t physical(uint16_t seg, uint16_t off) {
        return ((uint32_t(seg) << 4) + off) & 0xFFFFF;
    }
};

}  // namespace i8086
}  // namespace debugger
#endif /* __INST_I8086_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
