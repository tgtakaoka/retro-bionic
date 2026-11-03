#include "identity.h"

#include "pins_i80186.h"

namespace debugger {
namespace n80c186 {

Pins *instance() {
    return new i8086::PinsI80186(false);
}

TARGET_OVERLAY(i8086);

const struct Identity N80C186{"N80C186", instance, overlay};

}  // namespace n80c186
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
