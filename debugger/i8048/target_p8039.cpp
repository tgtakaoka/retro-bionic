#include "target.h"

#include "pins_i8048.h"

namespace debugger {
namespace p8039 {

Target *instanceP8039(const Identity *id) {
    return new Target(id, new i8048::PinsI8048());
}

const struct Identity P8039{"P8039", instanceP8039};

}  // namespace p8039
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
