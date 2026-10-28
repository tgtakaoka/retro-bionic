#include "identity.h"

#include "pins_i8086.h"

namespace debugger {
namespace i8086 {

Pins *instance() {
    return new PinsI8086();
}

TARGET_OVERLAY(i8086);

const struct Identity P8086{"P8086", instance, overlay};

}  // namespace i8086
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
