#include "identity.h"

#include "pins_z380.h"

namespace debugger {
namespace z380 {

Pins *instance() {
    return new PinsZ380();
}

const struct Identity Z380{"Z380", instance};

}  // namespace z380
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
