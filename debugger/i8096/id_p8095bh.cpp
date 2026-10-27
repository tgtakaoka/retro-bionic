#include "identity.h"

#include "pins_p8095bh.h"

namespace debugger {
namespace p8095bh {

Pins *instance() {
    return new PinsP8095BH();
}

TARGET_OVERLAY(i8096);

const struct Identity P8095BH{"P8095BH", instance, overlay};

}  // namespace p8095bh
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
