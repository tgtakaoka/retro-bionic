#include "identity.h"

#include "pins_ns32016.h"

namespace debugger {
namespace ns32016 {

Pins *instance() {
    return new ns32016::PinsNs32016(false);
}

TARGET_OVERLAY(ns32016);

const struct Identity NS32016{"NS32016", instance, overlay};

}  // namespace ns32016
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
