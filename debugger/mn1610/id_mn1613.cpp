#include "identity.h"

#include "pins_mn1613.h"

namespace debugger {
namespace mn1613 {

Pins *instance() {
    return new PinsMn1613();
}

const struct Identity MN1613{"MN1613", instance};

}  // namespace mn1613
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
