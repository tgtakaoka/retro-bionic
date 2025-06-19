#include "signals_p8095bh.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_p8095bh.h"

namespace debugger {
namespace p8095bh {

bool Signals::getControl() {
    // CNTL_RD and CNTL_WR is active low
    constexpr auto BUS_INACTIVE = CNTL_RD | CNTL_WR;
    // CNTL_FETCH is active low
    cntl() = busRead(CNTL) | CNTL_FETCH;
    return (cntl() & BUS_INACTIVE) != BUS_INACTIVE;
}

}  // namespace p8095bh
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
