#include "identity.h"

#include "pins_mc146805e2.h"

namespace debugger {
namespace mc146805e2 {

Pins *instance() {
    return new PinsMc146805E2();
}

const struct Identity MC146805E2{"MC146805E2", instance};

}  // namespace mc146805e2
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
