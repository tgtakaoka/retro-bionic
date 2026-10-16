#include "identity.h"

#include "pins_tms320c25.h"

namespace debugger {
namespace tms320c25 {

Pins *instance() {
    return new PinsTms320C25();
}

TARGET_OVERLAY(tms320);

const struct Identity TMS320C25{"TMS320C25", instance, overlay};

}  // namespace tms320c25
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
