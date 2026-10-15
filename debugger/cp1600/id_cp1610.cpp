#include "identity.h"

#include "pins_cp1610.h"

namespace debugger {
namespace cp1600 {

Pins *instance() {
    return new PinsCp1610();
}

const struct Identity CP1610{"CP1610", instance};

}  // namespace cp1600
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
