#include "identity.h"

#include "pins_ins8070.h"

namespace debugger {
namespace ins8070 {

Pins *instance() {
    return new PinsIns8070();
}

const struct Identity INS8070{"INS8070", instance};

}  // namespace ins8070
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
