#include "identity.h"

#include "pins_ns32016.h"

namespace debugger {
namespace ns32081 {

Pins *instance() {
    return new ns32016::PinsNs32016(true);
}

TARGET_OVERLAY(ns32016);

const struct Identity NS32081{"NS32081", instance, overlay};

}  // namespace ns32081
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
