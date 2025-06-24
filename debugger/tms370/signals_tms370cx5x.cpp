#include "signals_tms370cx5x.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_tms370cx5x.h"

namespace debugger {
namespace tms370cx5x {

void SignalsTms370Cx5x::getAddr() {
    addr = busRead(ADRL) | busRead(ADRM) | busRead(ADRH);
}

bool SignalsTms370Cx5x::getDirection() {
    cntl() = busRead(CNTL);
    return (cntl() & CNTL_EDS) == 0;
}

void SignalsTms370Cx5x::getData() {
    data = busRead(DATA);
}

void SignalsTms370Cx5x::outData() const {
    busWrite(DATA, data);
    busMode(DATA, OUTPUT);
}

void SignalsTms370Cx5x::inputMode() const {
    busMode(DATA, INPUT);
}

}  // namespace tms370cx5x
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
