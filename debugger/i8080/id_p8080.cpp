#include "identity.h"

#include "pins_i8080.h"

namespace debugger {
namespace i8080 {

Pins *instance() {
    return new PinsI8080();
}

const struct Identity P8080{"P8080", instance};

}  // namespace i8080
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
