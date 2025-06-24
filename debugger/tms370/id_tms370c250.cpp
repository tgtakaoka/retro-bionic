#include "identity.h"

#include "pins_tms370c250.h"

namespace debugger {
namespace tms370c250 {

Pins *instance() {
    return new PinsTms370C250();
}

const struct Identity TMS370C250{"TMS370C250", instance};

}  // namespace tms370c250
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
