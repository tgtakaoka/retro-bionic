#include "identity.h"

#include "pins_kl5c80.h"

namespace debugger {
namespace kl5c80 {

Pins *instance() {
    return new PinsKl5c80();
}

const struct Identity KL5C80A12{"KL5C80A12", instance};

}  // namespace kl5c80
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
