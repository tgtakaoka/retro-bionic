#ifndef __INST_Z280_H__
#define __INST_Z280_H__

#include <stdint.h>

namespace debugger {
namespace z280 {

struct InstZ280 {
    static constexpr uint8_t HALT = 0x76;
    static constexpr uint8_t RETN_PREFIX = 0xED;
    static constexpr uint8_t RETN = 0x45;
    // A Z-BUS fetch is a word, so RETN is injected as one.
    static constexpr uint16_t RETN_WORD = RETN_PREFIX << 8 | RETN;

    static constexpr uint16_t ORG_RESET = 0x0000;
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
