#ifndef __INST_Z80_H__
#define __INST_Z80_H__

#include <stdint.h>

namespace debugger {
namespace z80 {

struct InstZ80 {
    static constexpr uint8_t NOP = 0x00;
    static constexpr uint8_t JR = 0x18;
    static constexpr uint8_t JR_HERE = 0xFE;
    static constexpr uint8_t RETN_PREFIX = 0xED;
    static constexpr uint8_t RETN = 0x45;
    static constexpr uint8_t HALT = 0x77;
    static constexpr uint8_t RET = 0xC9;
    static constexpr uint8_t RST38H = 0xFF;

    static constexpr uint16_t ORG_RESET = 0x0000;
    static constexpr uint16_t ORG_RST38H = 0x0038;
    static constexpr uint16_t ORG_NMI = 0x0066;
};

}  // namespace z80
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
