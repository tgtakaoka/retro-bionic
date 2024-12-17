#include "target.h"

#include "pins_i8051.h"

namespace debugger {
namespace i8051 {

Target *instanceP8051(const Identity *id) {
    return new Target(id, new PinsI8051());
}

const struct Identity P8051{"P8051", instanceP8051};

}  // namespace i8051
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
