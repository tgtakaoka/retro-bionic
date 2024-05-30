#include "signals_nsc800.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_nsc800_config.h"

namespace debugger {
namespace nsc800 {

void SignalsNsc800::getAddress() {
    addr = busRead(ADDR);
}

void SignalsNsc800::getControl() {
    const uint8_t iom = digitalReadFast(PIN_IOM) ? CNTL_IOM : 0;
    nsc800() = busRead(CNTL) | iom;
    fetch() = (nsc800() & CNTL_S);
}

bool SignalsNsc800::memory() const {
    return (nsc800() & CNTL_IOM) == 0;
}

bool SignalsNsc800::intack() const {
    return (nsc800() & CNTL_INTA) == 0;
}

void SignalsNsc800::getData() {
    data = busRead(AD);
}

void SignalsNsc800::outData() const {
    busMode(AD, OUTPUT);
    busWrite(AD, data);
}

void SignalsNsc800::inputMode() const {
    busMode(AD, INPUT);
}

}  // namespace nsc800
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
