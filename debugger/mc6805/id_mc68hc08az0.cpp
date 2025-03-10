#include "identity.h"

#include "pins_mc68hc08az0.h"

namespace debugger {
namespace mc68hc08az0 {

Pins *instance() {
    return new PinsMc68HC08AZ0();
}

const struct Identity MC68HC08AZ0{"MC68HC08AZ0", instance};

}  // namespace mc68hc08az0
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
