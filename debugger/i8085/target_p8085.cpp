#include "target.h"

#include "pins_i8085.h"

namespace debugger {
namespace i8085 {

Target *instanceP8085(const Identity *id) {
    return new Target(id, new PinsI8085());
}

const struct Identity P8085{"P8085", instanceP8085};

}  // namespace i8085
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
