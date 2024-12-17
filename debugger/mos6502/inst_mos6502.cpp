#include "inst_mos6502.h"
#include "pins_mos6502.h"

namespace debugger {
namespace mos6502 {

bool InstMos6502::isStop(uint8_t inst) {
    return PinsMos6502::hardwareType() != HardwareType::HW_MOS6502 &&
           (inst == WAI || inst == STP);
}

}  // namespace mos6502
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
