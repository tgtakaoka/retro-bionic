#include "target.h"

#include "pins_f3850.h"

namespace debugger {
namespace f3850 {

Target *instanceF3850(const Identity *id) {
    return new Target(id, new PinsF3850());
}

const struct Identity F3850{"F3850", instanceF3850};

}  // namespace f3850
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
