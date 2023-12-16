#ifndef __INST_F3850_H__
#define __INST_F3850_H__

#include <stdint.h>

namespace debugger {
namespace f3850 {

struct InstF3850 final {
    static uint8_t instLength(uint8_t inst);
    static uint8_t busCycles(uint8_t inst);

    static constexpr uint8_t BREAK = 0x2F;
    static constexpr uint8_t BR = 0x90;
    static constexpr uint8_t BR_HERE = 0xFF;
};

}  // namespace f3850
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
