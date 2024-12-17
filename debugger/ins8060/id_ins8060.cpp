#include "identity.h"

#include "pins_ins8060.h"

namespace debugger {
namespace ins8060 {

Pins *instance() {
    return new PinsIns8060();
}

const struct Identity INS8060{"INS8060", instance};

}  // namespace ins8060
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
