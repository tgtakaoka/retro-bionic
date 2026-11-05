#include "identity.h"

#include "pins_mc68008.h"

namespace debugger {
namespace mc68008p {

Pins *instance() {
    return new mc68000::PinsMc68008(false);
}

TARGET_OVERLAY(mc68000);

const struct Identity MC68008P{"MC68008P", instance, overlay};

}  // namespace mc68008p
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
