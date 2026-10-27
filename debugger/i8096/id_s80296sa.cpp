#include "identity.h"

#include "pins_i80296sa.h"

namespace debugger {
namespace s80296sa {

Pins *instance() {
    return new PinsI80296SA(i8096::CPU_80296SA);
}

TARGET_OVERLAY(i8096);

const struct Identity S80296SA{"S80296SA", instance, overlay};

}  // namespace s80296sa
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
