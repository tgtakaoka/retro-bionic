#include "identity.h"

#include "pins_z180.h"

namespace debugger {
namespace hd64180s {

Pins *instance() {
    return new z180::PinsZ180();
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
