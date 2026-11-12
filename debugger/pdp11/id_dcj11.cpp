#include "identity.h"

#include "pins_dcj11.h"

namespace debugger {
namespace dcj11 {

Pins *instance() {
    return new PinsDcj11();
}

TARGET_OVERLAY(pdp11);

const struct Identity DCJ11{"DCJ11", instance, overlay};

}  // namespace dcj11
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
