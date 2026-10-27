#include "identity.h"

#include "pins_tms9980.h"

namespace debugger {
namespace tms9981 {

Pins *instance() {
    return new tms9980::PinsTms9980();
}

TARGET_OVERLAY(tms9900);

const struct Identity TMS9981{"TMS9981", instance, overlay};

}  // namespace tms9981
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
