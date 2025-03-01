#include "identity.h"

#include "mc68hc11_init.h"
#include "pins_mc68hc11.h"

namespace debugger {
namespace mc68hc11a {

struct mc68hc11::Mc68hc11Init Init{0x01, 0x00, 256, 0x3F};

Pins *instance() {
    return new mc68hc11::PinsMc68hc11(Init);
}

const struct Identity MC68HC11A{"MC68HC11A", instance};

}  // namespace mc68hc11a
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
