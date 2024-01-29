#include "signals_mc6802.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_mc6802.h"

namespace debugger {
namespace mc6802 {

void Signals::getAddr() {
    addr = busRead(AL) | busRead(AM) | busRead(AH);
}

void Signals::getDirection() {
    rw() = digitalReadFast(PIN_RW);
    vma() = digitalReadFast(PIN_VMA);
    clearFetch();
}

void Signals::getData() {
    data = busRead(D);
}

}  // namespace mc6802
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
