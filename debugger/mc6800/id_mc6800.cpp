#include "identity.h"

#include "pins_mc6800.h"

namespace debugger {
namespace mc6800 {

Pins *instance() {
    return new PinsMc6800();
}

TARGET_OVERLAY(mc6800);

const struct Identity MC6800{"MC6800", instance, overlay};

}  // namespace mc6800
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
