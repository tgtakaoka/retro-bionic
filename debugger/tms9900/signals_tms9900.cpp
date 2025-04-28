#include "signals_tms9900.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_tms9900.h"

namespace debugger {
namespace tms9900 {

void SignalsTms9900::getAddress() {
    digitalWriteFast(PIN_SEL0, LOW);
    delayNanoseconds(2);
    addr = busRead(ADR0);
    addr |= busRead(ADR1);
    digitalWriteFast(PIN_SEL0, HIGH);
    delayNanoseconds(2);
    addr |= busRead(ADR2);
    addr |= busRead(ADR3);
}

void SignalsTms9900::getControl() {
    // CNTL_MEMEN is active low
    // CNTL_DBIN is active high
    // CNTL_IAQ is active high
    cntl() = busRead(CNTL) | CNTL_16BIT;
}

void SignalsTms9900::getData() {
    data = busRead(DATA);
}

void SignalsTms9900::outData() const {
    busWrite(DATA, data);
    busMode(DATA, OUTPUT);
}

void SignalsTms9900::inputMode() const {
    busMode(DATA, INPUT);
}

}  // namespace tms9900
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
