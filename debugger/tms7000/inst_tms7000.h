#ifndef __INST_TMS7000_H__
#define __INST_TMS7000_H__

#include <stdint.h>

namespace debugger {
namespace tms7000 {

struct InstTms7000 {
    static constexpr uint8_t NOP = 0x00;
    static constexpr uint8_t IDLE = 0x01;
    static constexpr uint8_t JMP = 0xE0;
    static constexpr uint8_t JMP_HERE = 0xFE;
    static constexpr uint16_t VEC_RESET = 0xFFFE;

    static uint8_t busCycles(uint8_t opc);
    static bool isBTJxx(uint8_t opc) {
        return (opc & 0xE) == 6 && (opc >= 0x10 && opc < 0xB0);
    }
};

}  // namespace tms7000
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
