#include "identity.h"

#include "pins_mos6502.h"

namespace debugger {
namespace mos6502 {

Pins *instance() {
    return new PinsMos6502();
}

const struct Identity MOS6502{"MOS6502", instance};

}  // namespace mos6502
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
