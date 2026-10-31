#include "identity.h"

#include "pins_h8500.h"

namespace debugger {
namespace h8510 {

Pins *instance() {
    return new h8500::PinsH8500(h8500::H8_510);
}

TARGET_OVERLAY(h8500);

const struct Identity H8510{h8500::H8_510.name, instance, overlay};

}  // namespace h8510
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
