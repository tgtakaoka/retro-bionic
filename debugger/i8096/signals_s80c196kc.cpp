#include "signals_s80c196kc.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_s80c196kc.h"

namespace debugger {
namespace s80c196kc {

bool Signals::getControl() {
    // CNTL_RD and CNTL_WR is active low
    constexpr auto BUS_INACTIVE = CNTL_RD | CNTL_WR;
    // INST rides CNTL4
    cntl() = busRead(CNTL);
    return (cntl() & BUS_INACTIVE) != BUS_INACTIVE;
}

}  // namespace s80c196kc
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
