#include "target.h"

#include "pins_mc146805e2.h"

namespace debugger {
namespace mc146805e2 {

Target *instanceMC146805E2(const Identity *id) {
    return new Target(id, new PinsMc146805E2());
}

const struct Identity MC146805E2{"MC146805E2", instanceMC146805E2};

}  // namespace mc146805e2
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
