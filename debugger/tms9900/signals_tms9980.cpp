#include "signals_tms9980.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_tms9980.h"

namespace debugger {
namespace tms9980 {

void SignalsTms9980::getLowAddr() {
    addr = busRead(AL);
}

void SignalsTms9980::getHighAddr() {
    addr |= busRead(AM) | busRead(AH);
}

void SignalsTms9980::getControl() {
    // CNTL_MEMEN is active low
    // CNTL_DBIN is active high
    // CNTL_IAQ is active high
    cntl() = busRead(CNTL);
}

void SignalsTms9980::getData() {
    data = busRead(DATA);
}

void SignalsTms9980::outData() const {
    busWrite(DATA, data);
    busMode(DATA, OUTPUT);
}

void SignalsTms9980::inputMode() const {
    busMode(DATA, INPUT);
}

}  // namespace tms9980
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
