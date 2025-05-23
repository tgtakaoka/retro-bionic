#include "identity.h"

#include "pins_tms9900.h"

namespace debugger {
namespace tms9900 {

Pins *instance() {
    return new PinsTms9900();
}

const struct Identity TMS9900{"TMS9900", instance};

}  // namespace tms9900
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
