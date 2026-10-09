#include "identity.h"

#include "pins_z8000.h"

namespace debugger {
namespace z8002 {

Pins *instance() {
    return new z8000::PinsZ8000();
}

const struct Identity Z8002{"Z8002", instance};

}  // namespace z8002
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
