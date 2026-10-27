#include "identity.h"

#include "pins_mc68hc16.h"

namespace debugger {
namespace mc68hc16z {

Pins *instance() {
    return new mc68hc16::PinsMc68hc16();
}

TARGET_OVERLAY(mc68hc16);

const struct Identity MC68HC16Z{"MC68HC16Z", instance, overlay};

}  // namespace mc68hc16z
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
