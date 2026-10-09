#include "signals_n8097bh.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_n8097bh.h"

namespace debugger {
namespace n8097bh {

bool Signals::getControl() {
    // CNTL_RD and CNTL_WR is active low
    constexpr auto BUS_INACTIVE = CNTL_RD | CNTL_WR;
    // INST rides CNTL4
    cntl() = busRead(CNTL);
    return (cntl() & BUS_INACTIVE) != BUS_INACTIVE;
}

}  // namespace n8097bh
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
