#include "signals_mc6801.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_mc6801.h"

namespace debugger {
namespace mc6801 {

void Signals::getAddr() {
    addr = busRead(ADDR);
}

void Signals::getDirection() {
    cntl() = busRead(CNTL) | CNTL_VMA;
    clearFetch();
}

void Signals::getData() {
    data = busRead(AD);
}

void Signals::setData() const {
    busWrite(AD, data);
}

void Signals::outputMode() {
    busMode(AD, OUTPUT);
}

void Signals::inputMode() {
    busMode(AD, INPUT);
}

}  // namespace mc6801
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
