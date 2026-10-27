#include "identity.h"

#include "pins_i8085.h"

namespace debugger {
namespace i8085 {

Pins *instance() {
    return new PinsI8085();
}

TARGET_OVERLAY(i8080);

const struct Identity P8085{"P8085", instance, overlay};

}  // namespace i8085
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
