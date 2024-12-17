#include "target.h"

#include "pins_mc6809e.h"

namespace debugger {
namespace mc6809e {

Target *instanceMC6809E(const Identity *id) {
    return new Target(id, new PinsMc6809E());
}

const struct Identity MC6809E{"MC6809E", instanceMC6809E};

}  // namespace mc6809e
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
