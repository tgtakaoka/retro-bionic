#include "pins_z80_base.h"
#include "devs_z80.h"
#include "inst_z80.h"
#include "mems_z80.h"
#include "regs_z80.h"

namespace debugger {
namespace z80 {

PinsZ80Base::PinsZ80Base() {
    auto regs = new RegsZ80(this);
    _regs = regs;
    _devs = new DevsZ80();
    _mems = new MemsZ80(regs);
}

void PinsZ80Base::execInst(const uint8_t *inst, uint_fast8_t len) {
    execute(inst, len, nullptr, 0);
}

uint16_t PinsZ80Base::captureWrites(
        const uint8_t *inst, uint_fast8_t len, uint8_t *buf, uint_fast8_t max) {
    return execute(inst, len, buf, max);
}

void PinsZ80Base::setBreakInst(uint32_t addr) const {
    _mems->put_inst(addr, InstZ80::HALT);
}

}  // namespace z80
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
