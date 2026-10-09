#include "identity.h"

#include "pins_z8000.h"

namespace debugger {
namespace z8001 {

Pins *instance() {
    return new z8000::PinsZ8000(true);
}

const struct Identity Z8001{"Z8001", instance};

}  // namespace z8001
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
