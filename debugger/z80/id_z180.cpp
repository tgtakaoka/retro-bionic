#include "identity.h"

#include "pins_z180.h"

namespace debugger {
namespace z180 {

Pins *instance() {
    return new PinsZ180();
}

const struct Identity Z180{"Z180", instance};

}  // namespace z180
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
