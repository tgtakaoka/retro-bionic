#include "signals_tms9995.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_tms9995.h"

namespace debugger {
namespace tms9995 {

void Signals::getAddress() {
    addr = busRead(AL) | busRead(AM) | busRead(AH);
}

void Signals::getControl() {
    // CNTL_MEMEN is active low
    // CNTL_DBIN is active low
    // CNTL_IAQ is active high
    cntl() = busRead(CNTL) ^ CNTL_DBIN;
}

void Signals::getData() {
    data = busRead(DATA);
}

void Signals::outData() const {
    busWrite(DATA, data);
    busMode(DATA, OUTPUT);
}

void Signals::inputMode() const {
    busMode(DATA, INPUT);
}

}  // namespace tms9995
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
