#include "identity.h"

#include "pins_n8097bh.h"

namespace debugger {
namespace n8097bh {

Pins *instance() {
    return new PinsN8097BH();
}

TARGET_OVERLAY(i8096);

const struct Identity N8097BH{"N8097BH", instance, overlay};

}  // namespace n8097bh
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
