#include "target.h"

#include "pins_mc6800.h"

namespace debugger {
namespace mc6800 {

Target *instanceMC6800(const Identity *id) {
    return new Target(id, new PinsMc6800());
}

const struct Identity MC6800{"MC6800", instanceMC6800};

}  // namespace mc6800
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
