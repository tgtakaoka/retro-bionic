#include "identity.h"

#include "pins_kl5c8400.h"

namespace debugger {
namespace kl5c8400 {

Pins *instance() {
    return new PinsKl5c8400();
}

TARGET_OVERLAY(z80);

const struct Identity KL5C8400{"KL5C8400", instance, overlay};

}  // namespace kl5c8400
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
