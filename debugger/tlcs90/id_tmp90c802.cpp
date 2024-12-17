#include "identity.h"

#include "pins_tlcs90.h"

namespace debugger {
namespace tlcs90 {

Pins *instance() {
    return new PinsTlcs90();
}

const struct Identity TMP90C802P{"TMP90C802", instance};

}  // namespace tlcs90
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
