#include "identity.h"

#include "pins_mc68000.h"

namespace debugger {
namespace mc68000 {

Pins *instanceP() {
    return new PinsMc68000P();
}

Pins *instanceFn() {
    return new PinsMc68000Fn();
}

TARGET_OVERLAY(mc68000);

const struct Identity MC68000P{"MC68000P", instanceP, overlay};
const struct Identity HD68000PS{"HD68000PS", instanceP, overlay};
const struct Identity MC68000FN{"MC68000FN", instanceFn, overlay};

}  // namespace mc68000
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
