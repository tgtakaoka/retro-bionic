#include "identity.h"

#include "pins_tms7000.h"

namespace debugger {
namespace tms7000 {

Pins *instance() {
    return new PinsTms7000();
}

const struct Identity TMS7000{"TMS7000", instance};

}  // namespace tms7000
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
