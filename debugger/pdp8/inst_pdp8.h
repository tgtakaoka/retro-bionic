#ifndef __INST_PDP8_H__
#define __INST_PDP8_H__

#include <stdint.h>

namespace debugger {
namespace pdp8 {
struct InstPdp8 final {
    static constexpr uint16_t HLT = 07402;
    static bool isHalt(uint16_t inst) { return (inst & HLT) == HLT; }

    static constexpr uint16_t SAVED_PC = 00000;
    static constexpr uint16_t ORG_INTR = 00001;
    static constexpr uint16_t ORG_RESET = 07777;
};
}  // namespace pdp8
}  // namespace debugger

#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
