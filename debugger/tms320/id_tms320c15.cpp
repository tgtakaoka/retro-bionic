#include "identity.h"

#include "pins_tms320c15.h"

namespace debugger {
namespace tms320c15 {

Pins *instance() {
    return new PinsTms320C15();
}

const struct Identity TMS320C15{"TMS320C15", instance};

}  // namespace tms320c15
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
