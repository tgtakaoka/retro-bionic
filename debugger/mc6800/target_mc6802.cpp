#include "target.h"

#include "pins_mc6802.h"

namespace debugger {
namespace mc6802 {

Target *instanceMC6802(const Identity *id) {
    return new Target(id, new PinsMc6802());
}

const struct Identity MC6802{"MC6802", instanceMC6802};

}  // namespace mc6802
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
