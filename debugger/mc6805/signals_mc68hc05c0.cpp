#include "signals_mc68hc05c0.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_mc68hc05c0.h"

namespace debugger {
namespace mc68hc05c0 {

void Signals::getControl() {
    cntl() = busRead(CNTL) ^ CNTL_LI;
}

void Signals::getAddr() {
    addr = busRead(ADDR) ^ (1 << 15);
}

void Signals::getData() {
    data = busRead(AD);
}

void Signals::outData() const {
    busWrite(AD, data);
    busMode(AD, OUTPUT);
    busMode(AD, INPUT);
}

bool Signals::nobus() const {
    constexpr auto BUS_ACTIVE = CNTL_RW | CNTL_RD;
    return (cntl() & BUS_ACTIVE) == BUS_ACTIVE;
}

}  // namespace mc68hc05c0
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
