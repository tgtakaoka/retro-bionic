#include "identity.h"

#include "pins_tms99105.h"

namespace debugger {
namespace tms99105 {

Pins *instance() {
    return new PinsTms99105();
}

const struct Identity TMS99105{"TMS99105", instance};

}  // namespace tms99105
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
