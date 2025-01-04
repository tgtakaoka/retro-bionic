#include "signals_im6100.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_im6100.h"

namespace debugger {
namespace im6100 {

void Signals::getAddress() {
    addr = busRead(DX);
}

bool Signals::getControl() {
    cntl() = busRead(CNTL);
    // #MEMSEL, #DEVSEL, #CPSEL, #SWSEL are active low
    const auto SELECT = (CNTL_MEMSEL | CNTL_DEVSEL | CNTL_CPSEL | CNTL_SWSEL);
    // Returns true if any #xxxSEL asserted.
    return (cntl() & SELECT) != SELECT;
}

void Signals::getDirection() {
    // XTC(READ), IFETCH are active high
    dir() = busRead(DIR) ^ (DIR_XTC | DIR_IFETCH);
}

void Signals::getData() {
    data = busRead(DX);
}

void Signals::outData() const {
    busWrite(DX, data);
    busMode(DX, OUTPUT);
}

void Signals::inputMode() {
    busMode(DX, INPUT);
}

void Signals::outIoc(uint8_t data) {
    busWrite(IOC, ioc() = data);
}

}  // namespace im6100
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
