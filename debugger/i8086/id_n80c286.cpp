#include "identity.h"

#include "pins_i80286.h"

namespace debugger {
namespace n80c286 {

Pins *instance() {
    return new i8086::PinsI80286();
}

TARGET_OVERLAY(i8086);

const struct Identity N80C286{"N80C286", instance, overlay};

}  // namespace n80c286
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
