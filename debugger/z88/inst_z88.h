#ifndef __INST_Z88_H__
#define __INST_Z88_H__

#include <stdint.h>

namespace debugger {
namespace z88 {

struct InstZ88 {
    static uint8_t instLen(uint8_t inst);
    static uint8_t busCycles(uint8_t inst);

    static constexpr uint8_t WFI = 0x3F;
    static constexpr uint8_t JR = 0x8B;
    static constexpr uint8_t JR_HERE = 0xFE;
};

}  // namespace z88
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
