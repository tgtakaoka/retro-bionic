#ifndef __INST_SCN2650_H__
#define __INST_SCN2650_H__

#include <stdint.h>

namespace debugger {
namespace scn2650 {

struct InstScn2650 {
    static uint8_t instLen(uint8_t inst);
    static uint8_t busCycles(uint8_t inst, uint8_t opr);
    static bool isBranch(uint8_t inst);
    static bool notTaken(uint16_t fetch, uint8_t len, uint16_t addr);

    static constexpr uint8_t HALT = 0x40;
    static constexpr uint8_t ZBSR = 0xBB;
};

}  // namespace scn2650
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
