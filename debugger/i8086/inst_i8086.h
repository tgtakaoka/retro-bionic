#ifndef __INST_I8086_H__
#define __INST_I8086_H__

#include <stdint.h>

namespace debugger {
namespace i8086 {

struct InstI8086 {
    static constexpr uint8_t INT3 = 0xCC;  // a breakpoint
    static constexpr uint8_t HLT = 0xF4;
    static constexpr uint8_t HLT_8080 = 0x76;
    static constexpr uint8_t IRET = 0xCF;
    static constexpr uint8_t JMPF = 0xEA;  // JMP ptr16:16
    static constexpr uint8_t NOP = 0x90;
    static constexpr uint8_t WAIT = 0x9B;
    static constexpr uint8_t PUSHF = 0x9C;

    // Reset starts at FFFF:0000 (8086 datasheet, Table 2).
    static constexpr uint16_t RESET_CS = 0xFFFF;
    static constexpr uint16_t RESET_IP = 0x0000;
    // The 80286 fetches first at FFFFF0H, CS=F000H based at FF0000H until
    // a far jump; F000:FFF0 is the 8086's reset address (M80C286
    // datasheet, Table 6).
    static constexpr uint32_t RESET_286 = 0xFFFFF0;
    static constexpr uint16_t RESET_CS_286 = 0xF000;
    static constexpr uint16_t RESET_IP_286 = 0xFFF0;
    // The 80286's A0-A23.
    static constexpr uint32_t ADDR_MASK_24 = 0xFFFFFF;

    // The vectors the debugger serves: single step, NMI and INT 3.
    static constexpr uint8_t VEC_STEP = 1;
    static constexpr uint8_t VEC_NMI = 2;
    static constexpr uint8_t VEC_BREAK = 3;
    static constexpr uint32_t VECTORS = 4 * VEC_STEP;
    static constexpr uint32_t VECTORS_END = 4 * (VEC_BREAK + 1);
    static constexpr bool vector(uint32_t addr) {
        return addr >= VECTORS && addr < VECTORS_END;
    }

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
