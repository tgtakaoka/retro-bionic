#include "mc6801_sci_handler.h"
#include "pins_mc6801.h"
#include "regs_mc6801.h"

namespace debugger {
namespace mc6801 {

struct Mc6801SciHandler SciH;

Mc6801SciHandler::Mc6801SciHandler()
    : SerialHandler(PIN_SCIRXD, PIN_SCITXD), _rmcr(0) {}

const char *Mc6801SciHandler::name() const {
    return "SCI";
}

const char *Mc6801SciHandler::description() const {
    return Regs.cpuName();
}

bool Mc6801SciHandler::isSelected(uint32_t addr) const {
    return addr == ADDR_RMCR;
}

void Mc6801SciHandler::write(uint32_t addr, uint16_t data) {
    _rmcr = data;
    resetHandler();
}

void Mc6801SciHandler::resetHandler() {
    static const uint16_t scaler[] = {16, 128, 1024, 4096};
    _pre_divider = scaler[_rmcr & RMCR_SS_gm] / 8;
    _divider = 8;
}

}  // namespace mc6801
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
