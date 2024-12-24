#ifndef __INST_TMS9900_H__
#define __INST_TMS9900_H__

#include <stdint.h>

namespace debugger {
namespace tms9900 {

struct InstTms9900 {
    static constexpr uint16_t VEC_RESET = 0x0000;
    static constexpr uint16_t VEC_XOP15 = 0x007C;
    static constexpr uint16_t XOP15 = 0x2FCF;  // XOP R15,15
};

}  // namespace tms9900
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
