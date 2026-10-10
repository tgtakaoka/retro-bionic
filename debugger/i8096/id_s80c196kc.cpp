#include "identity.h"

#include "pins_s80c196kc.h"

namespace debugger {
namespace s80c196kc {

Pins *instance() {
    return new PinsS80C196KC(i8096::CPU_80C196KC);
}

const struct Identity S80C196KC{"S80C196KC", instance};

}  // namespace s80c196kc
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
