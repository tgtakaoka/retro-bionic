#include "identity.h"

#include "pins_mc6801.h"

namespace debugger {
namespace mc6801 {

Pins *instance() {
    return new PinsMc6801();
}

TARGET_OVERLAY(mc6800);

const struct Identity MC6801{"MC6801", instance, overlay};

}  // namespace mc6801
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
