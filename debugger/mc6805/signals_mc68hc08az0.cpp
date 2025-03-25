#include "signals_mc68hc08az0.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_mc68hc08az0.h"

namespace debugger {
namespace mc68hc08az0 {

bool Signals::getControl() {
    // REB and WEB are active low
    constexpr auto BUS_ACTIVE = CNTL_REB | CNTL_WEB;
    cntl() = busRead(CNTL);
    return (cntl() & BUS_ACTIVE) != BUS_ACTIVE;
}

void Signals::getAddr() {
    addr = busRead(EAL) | busRead(EAM) | busRead(EAH);
}

void Signals::getData() {
    data = busRead(ED);
}

void Signals::outData() const {
    busWrite(ED, data);
    busMode(ED, OUTPUT);
}

void Signals::inputMode() {
    busMode(ED, INPUT);
}

}  // namespace mc68hc08az0
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
