#include "signals_mc146805e2.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_mc146805e2.h"

namespace debugger {
namespace mc146805e2 {

void Signals::getControl() {
    cntl() = busRead(CNTL);
}

void Signals::getAddr() {
    addr = busRead(ADDR);
}

void Signals::getData() {
    data = busRead(B);
}

void Signals::outData() const {
    busWrite(B, data);
    busMode(B, OUTPUT);
}

void Signals::inputMode() {
    busMode(B, INPUT);
}

}  // namespace mc146805e2
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
