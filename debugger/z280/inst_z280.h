#ifndef __INST_Z280_H__
#define __INST_Z280_H__

#include <stdint.h>

namespace debugger {
namespace z280 {

struct InstZ280 {
    static constexpr uint8_t NOP = 0x00;
    static constexpr uint8_t HALT = 0x76;
    static constexpr uint8_t RETN_PREFIX = 0xED;
    static constexpr uint8_t RETN = 0x45;
    static constexpr uint8_t RETIL = 0x55;  // ED 55: pops MSR and PC
    // A Z-BUS fetch is a word, so RETN is injected as one.
    static constexpr uint16_t RETN_WORD = RETN_PREFIX << 8 | RETN;

    static constexpr uint8_t RET = 0xC9;
    static constexpr uint8_t JP = 0xC3;
    // A program hands control back to the monitor by writing RST 38H
    // into the restart vector and restarting to it. Shared convention
    // with the Z80 targets, so the samples work unchanged.
    static constexpr uint8_t RST38 = 0xFF;

    static constexpr uint16_t ORG_RESET = 0x0000;
    static constexpr uint16_t ORG_RST38 = 0x0038;
    // #NMI pushes PC and vectors here in interrupt modes 0, 1 and 2.
    static constexpr uint16_t ORG_NMI = 0x0066;
};

}  // namespace z280
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
