#include "identity.h"

#include "pins_h8500.h"

namespace debugger {
namespace h8536 {

Pins *instance() {
    return new h8500::PinsH8500(h8500::H8_536);
}

TARGET_OVERLAY(h8500);

const struct Identity H8536{h8500::H8_536.name, instance, overlay};

}  // namespace h8536
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
