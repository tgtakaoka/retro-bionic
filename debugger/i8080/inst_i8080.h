#ifndef __INST_I8080_H__
#define __INST_I8080_H__

#include <stdint.h>

namespace debugger {
namespace i8080 {

struct InstI8080 {
    static constexpr uint8_t NOP = 0x00;
    static constexpr uint8_t HLT = 0x76;
    static constexpr uint8_t RET = 0xC9;
    static constexpr uint8_t DI = 0xF3;
    static constexpr uint8_t EI = 0xFB;
    static uint8_t vec2Inst(uint8_t vec) { return RST0 | (vec & 0x38); }

private:
    static constexpr uint8_t RST0 = 0xC7;
};

}  // namespace i8080
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
