#include "identity.h"

#include "pins_i8048.h"

namespace debugger {
namespace p8039 {

Pins *instance() {
    return new i8048::PinsI8048();
}

const struct Identity P8039{"P8039", instance};

}  // namespace p8039
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
