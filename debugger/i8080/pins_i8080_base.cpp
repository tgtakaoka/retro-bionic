#include "pins_i8080_base.h"
#include "inst_i8080.h"
#include "mems.h"

namespace debugger {
namespace i8080 {

void PinsI8080Base::execInst(const uint8_t *inst, uint_fast8_t len) {
    captureWrites(inst, len, nullptr, 0);
}

void PinsI8080Base::setBreakInst(uint32_t addr) const {
    _mems->put_prog(addr, InstI8080::HLT);
}

}  // namespace i8080
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
