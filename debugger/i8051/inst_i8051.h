#ifndef __INST_I8051_H__
#define __INST_I8051_H__

#include <stdint.h>

namespace debugger {
namespace i8051 {

struct InstI8051 final {
    static uint8_t instLength(uint8_t inst);
    static uint8_t busCycles(uint8_t inst);

    static constexpr uint8_t NOP = 0x00;
    static constexpr uint8_t SJMP = 0x80;
    static constexpr uint8_t SJMP_HERE = 0xFE;
    static constexpr uint8_t UNKNOWN = 0xA5;
};

}  // namespace i8051
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
