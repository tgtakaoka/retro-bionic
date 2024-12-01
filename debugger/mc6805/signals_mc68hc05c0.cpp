#include "signals_mc68hc05c0.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_mc68hc05c0.h"

namespace debugger {
namespace mc68hc05c0 {

void Signals::getDirection() {
    rw() = digitalReadFast(PIN_WR);
}

void Signals::getAddr() {
    addr = busRead(ADDR) ^ (1 << 15);
}

void Signals::getLoadInstruction() {
    li() = !digitalReadFast(PIN_LIR);
}

void Signals::getData() {
    data = busRead(AD);
}

}  // namespace mc68hc05c0
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
