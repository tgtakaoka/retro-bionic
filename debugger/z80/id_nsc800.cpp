#include "identity.h"

#include "pins_nsc800.h"

namespace debugger {
namespace nsc800 {

Pins *instance() {
    return new PinsNsc800();
}

const struct Identity NSC800{"NSC800", instance};

}  // namespace nsc800
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
