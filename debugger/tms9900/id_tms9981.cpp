#include "identity.h"

#include "pins_tms9981.h"

namespace debugger {
namespace tms9981 {

Pins *instance() {
    return new PinsTms9981();
}

const struct Identity TMS9981{"TMS9981", instance};

}  // namespace tms9981
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
