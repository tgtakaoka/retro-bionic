#ifndef __INST_Z86_H__
#define __INST_Z86_H__

#include <stdint.h>

namespace debugger {
namespace z86 {

struct InstZ86 {
    static uint8_t instLen(uint8_t inst);
    static uint8_t busCycles(uint8_t inst);

    static constexpr uint8_t STOP = 0x6F;
    static constexpr uint8_t HALT = 0x7F;
    static constexpr uint8_t JR = 0x8B;
    static constexpr uint8_t JR_HERE = 0xFE;
};

}  // namespace z86
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
