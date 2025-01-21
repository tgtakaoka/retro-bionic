#ifndef __INST_MN1613_H__
#define __INST_MN1613_H__

#include <stdint.h>

namespace debugger {
namespace mn1613 {

struct InstMn1613 {
    static constexpr uint16_t RESET_PSW = 0x0108;
    static constexpr uint16_t NOP = 0x7808;  // MV R0,R0
    static constexpr uint16_t H = 0x2000;
};

}  // namespace mn1613
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
