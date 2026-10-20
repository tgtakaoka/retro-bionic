#include "identity.h"

#include "mc9s12x_init.h"
#include "pins_mc9s12x.h"

namespace debugger {
namespace mc9s12x {

mc68hc12::Mc9s12xInit Init;

Pins *instance() {
    return new mc68hc12::PinsMc9s12x(Init);
}

const struct Identity MC9S12X{"MC9S12X", instance};

}  // namespace mc9s12x
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
