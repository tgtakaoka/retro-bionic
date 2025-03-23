#include "regs_tms9980.h"
#include "debugger.h"
#include "inst_tms9900.h"
#include "pins_tms9980.h"

namespace debugger {
namespace tms9980 {

using tms9900::InstTms9900;

RegsTms9980::RegsTms9980(PinsTms9900Base *pins, Mems *mems)
    : RegsTms9900(pins, mems) {}

const char *RegsTms9980::cpu() const {
    return "TMS9980";
}

void RegsTms9980::reset() {
    _wp = _mems->read16(InstTms9900::VEC_RESET + 0);
    _pc = _mems->read16(InstTms9900::VEC_RESET + 2);
}

uint16_t RegsTms9980::read_reg(uint8_t i) const {
    return _mems->read16(_wp + i * 2);
}

void RegsTms9980::write_reg(uint8_t i, uint16_t data) const {
    _mems->write16(_wp + i * 2, data);
}

}  // namespace tms9980
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
