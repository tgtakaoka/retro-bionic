#include "identity.h"

#include "pins_z80.h"

namespace debugger {
namespace z80 {

Pins *instance() {
    return new PinsZ80();
}

const struct Identity Z80{"Z80", instance};

}  // namespace z80
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
