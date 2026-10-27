#include "identity.h"

#include "pins_mc6802.h"

namespace debugger {
namespace mc6802 {

Pins *instance() {
    return new PinsMc6802();
}

TARGET_OVERLAY(mc6800);

const struct Identity MC6802{"MC6802", instance, overlay};

}  // namespace mc6802
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
