#include "signals_mc146805e2.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_mc146805e2.h"

namespace debugger {
namespace mc146805e2 {

void SignalsMc146805E2::getControl() {
#if CNTL_WRITE == CNTL_RW && CNTL_FETCH == CNTL_LI
    cntl() = busRead(CNTL) & ~CNTL_READ;
#else
#error "Check CNTL_ definitions"
#endif
}

void SignalsMc146805E2::getAddr() {
    addr = busRead(ADDR);
}

void SignalsMc146805E2::getData() {
    data = busRead(B);
}

void SignalsMc146805E2::outData() const {
    busWrite(B, data);
    busMode(B, OUTPUT);
}

void SignalsMc146805E2::inputMode() {
    busMode(B, INPUT);
}

}  // namespace mc146805e2
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
