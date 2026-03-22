#include "identity.h"

#include "pins_z280.h"

namespace debugger {
namespace z280 {

Pins *instance() {
    return new PinsZ280();
}

const struct Identity Z280{"Z280", instance};

}  // namespace z280
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
