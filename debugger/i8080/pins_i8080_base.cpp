#include "pins_i8080_base.h"
#include "debugger.h"
#include "inst_i8080.h"

namespace debugger {
namespace i8080 {

void PinsI8080Base::execInst(const uint8_t *inst, uint8_t len) {
    execute(inst, len, nullptr, nullptr, 0);
}

void PinsI8080Base::captureWrites(const uint8_t *inst, uint8_t len,
        uint16_t *addr, uint8_t *buf, uint8_t max) {
    execute(inst, len, addr, buf, max);
}

void PinsI8080Base::setBreakInst(uint32_t addr) const {
    _mems->put_inst(addr, InstI8080::HLT);
}

}  // namespace i8080
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
