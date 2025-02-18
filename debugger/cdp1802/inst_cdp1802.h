#ifndef __INST_CDP1802_H__
#define __INST_CDP1802_H__

#include <stdint.h>

namespace debugger {
namespace cdp1802 {
struct InstCdp1802 {
    static constexpr uint8_t IDL = 0x00;
    static constexpr uint8_t PREFIX = 0x68;
    static constexpr uint8_t RET = 0x70;
    static constexpr uint8_t DIS = 0x71;
    static constexpr uint8_t REQ = 0x7A;
    static constexpr uint8_t SEQ = 0x7B;
    static constexpr uint8_t PHI = 0xB0;
    static constexpr uint8_t PLO = 0xA0;
    static constexpr uint8_t LBR = 0xC0;
    static constexpr uint8_t NOP = 0xC4;
    static constexpr uint8_t LSIE = 0xCC;
    static constexpr uint8_t LSQ = 0xCD;
    static constexpr uint8_t LSDF = 0xCF;
    static constexpr uint8_t SEP = 0xD0;
    static constexpr uint8_t SEX = 0xE0;
    static constexpr uint8_t LDI = 0xF8;
    static constexpr uint8_t DADI = 0xFC;
};
}  // namespace cdp1802
}  // namespace debugger

#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
