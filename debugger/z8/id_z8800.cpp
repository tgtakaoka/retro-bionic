#include "identity.h"

#include "pins_z88.h"

namespace debugger {
namespace z88 {

Pins *instance() {
    return new PinsZ88();
}

const struct Identity Z88C00{"Z88C00", instance};

}  // namespace z88
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
