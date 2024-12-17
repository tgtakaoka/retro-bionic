#include "target.h"

#include "pins_mc6801.h"

namespace debugger {
namespace mc6801 {

Target *instanceMC6801(const Identity *id) {
    return new Target(id, new PinsMc6801());
}

const struct Identity MC6801{"MC6801", instanceMC6801};

}  // namespace mc6801
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
