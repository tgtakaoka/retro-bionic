#include "identity.h"

#include "pins_hd6120.h"

namespace debugger {
namespace hd6120 {

Pins *instance() {
    return new PinsHd6120();
}

const struct Identity HD6120{"HD6120", instance};

}  // namespace hd6120
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
