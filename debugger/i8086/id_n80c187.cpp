#include "identity.h"

#include "pins_i80186.h"

namespace debugger {
namespace n80c187 {

Pins *instance() {
    return new i8086::PinsI80186(true);
}

TARGET_OVERLAY(i8086);

const struct Identity N80C187{"N80C187", instance, overlay};

}  // namespace n80c187
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
