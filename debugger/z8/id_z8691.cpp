#include "identity.h"

#include "pins_z86.h"

namespace debugger {
namespace z86 {

Pins *instance() {
    return new PinsZ86();
}

const struct Identity Z86C91{"Z86C91", instance};

}  // namespace z86
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
