#include "signals_mc68hc11.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_mc68hc11.h"

namespace debugger {
namespace mc68hc11 {

void Signals::getAddr() {
    addr = busRead(ADDR);
}

void Signals::getDirection() {
    cntl() = busRead(CNTL) | CNTL_VMA;
}

void Signals::getControl() {
    fetch() = digitalReadFast(PIN_LIR) == LOW;
}

void Signals::getData() {
    data = busRead(AD);
}

void Signals::outData() const {
    busWrite(AD, data);
    busMode(AD, OUTPUT);
}

void Signals::inputMode() {
    busMode(AD, INPUT);
}

}  // namespace mc68hc11
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
