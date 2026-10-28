#include "identity.h"

#include "pins_dct11.h"

namespace debugger {
namespace dct11 {

Pins *instance() {
    return new PinsDct11();
}

TARGET_OVERLAY(pdp11);

const struct Identity DCT11{"DCT11", instance, overlay};

}  // namespace dct11
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
