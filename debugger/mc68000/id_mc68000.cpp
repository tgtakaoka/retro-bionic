#include "identity.h"

#include "pins_mc68000x16.h"

namespace debugger {
namespace mc68000 {

Pins *instance() {
    return new PinsMc68000x16();
}

TARGET_OVERLAY(mc68000);

const struct Identity MC68000{"MC68000", instance, overlay};

}  // namespace mc68000
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
