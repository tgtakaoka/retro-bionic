#include "identity.h"

#include "pins_mc68008.h"

namespace debugger {
namespace mc68008 {

Pins *instance() {
    return new mc68000::PinsMc68008();
}

TARGET_OVERLAY(mc68000);

const struct Identity MC68008{"MC68008", instance, overlay};

}  // namespace mc68008
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
