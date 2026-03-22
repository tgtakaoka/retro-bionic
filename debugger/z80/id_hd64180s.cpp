#include "identity.h"

#include "pins_hd64180s.h"

namespace debugger {
namespace hd64180s {

Pins *instance() {
    return new PinsHD64180S();
}

const struct Identity HD64180S{"HD64180S", instance};

}  // namespace hd64180s
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
