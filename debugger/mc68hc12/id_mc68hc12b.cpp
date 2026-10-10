#include "identity.h"

#include "mc68hc12_init.h"
#include "pins_mc68hc12.h"

namespace debugger {
namespace mc68hc12b {

mc68hc12::Mc68hc12Init Init;

Pins *instance() {
    return new mc68hc12::PinsMc68hc12(Init);
}

const struct Identity MC68HC12B{"MC68HC12B", instance};

}  // namespace mc68hc12b
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
