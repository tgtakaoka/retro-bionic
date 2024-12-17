#include "identity.h"

#include "pins_cdp1802.h"

namespace debugger {
namespace cdp1802 {

Pins *instance() {
    return new PinsCdp1802();
}

const struct Identity CDP1802{"CDP1802", instance};

}  // namespace cdp1802
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
