#include "identity.h"

#include "mc68hc11_init.h"
#include "pins_mc68hc11.h"

namespace debugger {
namespace mc68hc11d {

struct mc68hc11::Mc68hc11Init Init{0x00, 0x40, 192, 0x3F};

Pins *instance() {
    return new mc68hc11::PinsMc68hc11(Init);
}

const struct Identity MC68HC11D{"MC68HC11D", instance};

}  // namespace mc68hc11d
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
