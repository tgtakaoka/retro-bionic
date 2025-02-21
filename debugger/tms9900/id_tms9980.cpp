#include "identity.h"

#include "pins_tms9980.h"

namespace debugger {
namespace tms9980 {

Pins *instance() {
    return new PinsTms9980();
}

const struct Identity TMS9980{"TMS9980", instance};

}  // namespace tms9980
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
