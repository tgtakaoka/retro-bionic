#include "identity.h"

#include "mc9s12d_init.h"
#include "pins_mc9s12d.h"

namespace debugger {
namespace mc9s12d {

mc68hc12::Mc9s12dInit Init;

Pins *instance() {
    return new mc68hc12::PinsMc9s12d(Init);
}

TARGET_OVERLAY(mc68hc12);

const struct Identity MC9S12D{"MC9S12D", instance, overlay};

}  // namespace mc9s12d
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
