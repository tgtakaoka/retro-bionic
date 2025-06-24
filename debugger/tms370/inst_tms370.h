#ifndef __INST_TMS370_H__
#define __INST_TMS370_H__

#include <stdint.h>

namespace debugger {
namespace tms370 {

struct InstTms370 {
    static constexpr uint16_t VEC_RESET = 0x7FFE;
    static constexpr uint16_t VEC_TRAP15 = 0x7FC0;

    static constexpr uint8_t TRAP15 = 0xE0;
};

}  // namespace tms370
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
