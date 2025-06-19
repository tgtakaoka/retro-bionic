#include "identity.h"

#include "pins_p8095.h"

namespace debugger {
namespace p8095 {

Pins *instance() {
    return new PinsP8095();
}

const struct Identity P8095{"P8095", instance};

}  // namespace tms320c15
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
