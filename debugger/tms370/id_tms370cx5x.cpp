#include "identity.h"

#include "pins_tms370cx5x.h"

namespace debugger {
namespace tms370cx5x {

Pins *instance() {
    return new PinsTms370Cx5x();
}

const struct Identity TMS370Cx5x{"TMS370Cx5x", instance};

}  // namespace tms370cx5x
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
