#include "target.h"

#include "pins_ins8060.h"

namespace debugger {
namespace ins8060 {

Target *instanceINS8060(const Identity *id) {
    return new Target(id, new PinsIns8060());
}

const struct Identity INS8060{"INS8060", instanceINS8060};

}  // namespace ins8060
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
