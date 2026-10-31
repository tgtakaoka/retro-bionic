#include "identity.h"

#include "pins_h8300.h"

namespace debugger {
namespace h8300 {

Pins *instance() {
    return new PinsH8300();
}

TARGET_OVERLAY(h8300);

const struct Identity H8323{"H8/323", instance, overlay};

}  // namespace h8300
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
