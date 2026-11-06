#include "identity.h"

#include "pins_h16.h"

namespace debugger {
namespace hd641016 {

Pins *instance() {
    return new h16::PinsH16();
}

TARGET_OVERLAY(h16);

const struct Identity HD641016{"HD641016", instance, overlay};

}  // namespace hd641016
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
