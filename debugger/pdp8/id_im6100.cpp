#include "identity.h"

#include "pins_im6100.h"

namespace debugger {
namespace im6100 {

Pins *instance() {
    return new PinsIm6100();
}

TARGET_OVERLAY(pdp8);

const struct Identity IM6100{"IM6100", instance, overlay};

}  // namespace im6100
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
