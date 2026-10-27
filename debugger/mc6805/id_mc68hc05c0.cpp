#include "identity.h"

#include "pins_mc68hc05c0.h"

namespace debugger {
namespace mc68hc05c0 {

Pins *instance() {
    return new PinsMc68HC05C0();
}

TARGET_OVERLAY(mc6805);

const struct Identity MC68HC05{"MC68HC05C0", instance, overlay};

}  // namespace mc68hc05c0
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
