#include "identity.h"

#include "pins_s80c196kc.h"

namespace debugger {
namespace s80c196kc {

Pins *instance() {
    return new PinsS80C196KC(i8096::CPU_80C196KC);
}

// The S80C196KB or KC, told apart at reset.
TARGET_OVERLAY(i8096);

const struct Identity S80C196K{"S80C196K", instance, overlay};

}  // namespace s80c196kc
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
