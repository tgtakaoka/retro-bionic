#include "identity.h"

#include "pins_f3850.h"

namespace debugger {
namespace f3850 {

Pins *instance() {
    return new PinsF3850();
}

const struct Identity F3850{"F3850", instance};

}  // namespace f3850
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
