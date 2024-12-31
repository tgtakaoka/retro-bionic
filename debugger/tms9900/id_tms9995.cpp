#include "identity.h"

#include "pins_tms9995.h"

namespace debugger {
namespace tms9995 {

Pins *instance() {
    return new PinsTms9995();
}

const struct Identity TMS9995{"TMS9995", instance};

}  // namespace tms9995
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
