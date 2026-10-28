#include "identity.h"

#include "pins_i8086.h"

namespace debugger {
namespace d8087 {

Pins *instance() {
    return new i8086::PinsI8086(true);
}

TARGET_OVERLAY(i8086);

const struct Identity D8087{"D8087", instance, overlay};

}  // namespace d8087
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
