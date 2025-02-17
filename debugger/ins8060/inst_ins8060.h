#ifndef __INST_INS8060_H__
#define __INST_INS8060_H__

#include <stdint.h>

namespace debugger {
namespace ins8060 {

struct InstIns8060 {
    static uint8_t instLen(uint8_t inst);

    static constexpr uint8_t HALT = 0x00;
    static constexpr uint8_t JMP = 0x90;
    static constexpr uint8_t JMP_HERE = 0xFE;
    static constexpr uint8_t JMP_HALT = 0xFD;
};

}  // namespace ins8060
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
