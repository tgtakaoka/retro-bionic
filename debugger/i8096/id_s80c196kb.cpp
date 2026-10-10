#include "identity.h"

#include "pins_s80c196kc.h"

namespace debugger {
namespace s80c196kb {

using s80c196kc::PinsS80C196KC;

Pins *instance() {
    return new PinsS80C196KC(i8096::CPU_80C196KB);
}

const struct Identity S80C196KB{"S80C196KB", instance};

}  // namespace s80c196kb
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
