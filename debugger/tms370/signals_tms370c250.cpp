#include "signals_tms370c250.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_tms370c250.h"

namespace debugger {
namespace tms370c250 {

void SignalsTms370C250::getAddrLow() {
    addr = busRead(ADRL);
}

void SignalsTms370C250::getAddrMid() {
    addr |= busRead(ADRM);
}

void SignalsTms370C250::getAddrHigh() {
    addr |= busRead(ADRH);
}

bool SignalsTms370C250::getDirection() {
    cntl() = busRead(CNTL);
    return (cntl() & CNTL_EDS) == 0;
}

void SignalsTms370C250::getData() {
    data = busRead(DATA);
}

void SignalsTms370C250::outData() const {
    busWrite(DATA, data);
    busMode(DATA, OUTPUT);
}

void SignalsTms370C250::inputMode() const {
    busMode(DATA, INPUT);
}

}  // namespace tms370c250
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
