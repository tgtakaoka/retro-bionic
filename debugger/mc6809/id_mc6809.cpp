#include "identity.h"

#include "pins_mc6809.h"

namespace debugger {
namespace mc6809 {

Pins *instance() {
    return new PinsMc6809();
}

const struct Identity MC6809{"MC6809", instance};

}  // namespace mc6809
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
