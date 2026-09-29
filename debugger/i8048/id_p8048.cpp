#include "identity.h"

#include "pins_i8048.h"

namespace debugger {
namespace p8048 {

Pins *instance() {
    return new i8048::PinsI8048();
}

const struct Identity P8048{"P8048", instance};

}  // namespace p8048
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
