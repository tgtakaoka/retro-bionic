#include "signals_hd6120.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_hd6120.h"

namespace debugger {
namespace hd6120 {

void Signals::getAddress() {
    addr = busRead(ADDR);
}

bool Signals::getControl() {
    cntl() = busRead(CNTL) | CNTL_15BIT;
    // #LXMAR, #LXDAR, #LXPAR are active low.
    constexpr auto SELECT = (CNTL_LXMAR | CNTL_LXDAR | CNTL_LXPAR);
    // Returns true if any #LXxAR found
    return (cntl() & SELECT) != SELECT;
}

bool Signals::getDirection() {
    // #WRITE, #READ, #IFETCH are active low
    dir() = busRead(DIR);
    // #READ, #WRITE are active low
    constexpr auto DIR = (DIR_READ | DIR_WRITE);
    // Returns true if #READ or #WRITE found.
    return (dir() & DIR) != DIR;
}

void Signals::getData() {
    data = busRead(DATA);
}

void Signals::outData() const {
    busWrite(DATA, data);
    busMode(DATA, OUTPUT);
}

void Signals::inputMode() {
    busMode(DATA, INPUT);
}

void Signals::outIoc(uint8_t val) {
    ioc() = val;
    busWrite(IOC, ~val);
}

void Signals::resetIoc() {
    busWrite(IOC, 0);
}

}  // namespace hd6120
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
