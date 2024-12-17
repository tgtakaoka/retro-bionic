#include "pins_z80_base.h"
#include "devs_z80.h"
#include "inst_z80.h"
#include "mems_z80.h"
#include "regs_z80.h"

namespace debugger {
namespace z80 {

PinsZ80Base::PinsZ80Base(const char *name) {
    auto regs = new RegsZ80("Z80", this);
    _regs = regs;
    _devs = new DevsZ80();
    _mems = new MemsZ80(regs);
}

void PinsZ80Base::execInst(const uint8_t *inst, uint8_t len) {
    execute(inst, len, nullptr, nullptr, 0);
}

void PinsZ80Base::captureWrites(const uint8_t *inst, uint8_t len,
        uint16_t *addr, uint8_t *buf, uint8_t max) {
    execute(inst, len, addr, buf, max);
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
