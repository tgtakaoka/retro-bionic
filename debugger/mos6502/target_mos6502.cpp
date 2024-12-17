#include "target.h"

#include "pins_mos6502.h"

namespace debugger {
namespace mos6502 {

Target *instanceMOS6502(const Identity *id) {
    return new Target(id, new PinsMos6502());
}

const struct Identity MOS6502{"MOS6502", instanceMOS6502};

}  // namespace mos6502
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
