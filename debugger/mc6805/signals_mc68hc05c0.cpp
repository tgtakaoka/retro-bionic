#include "signals_mc68hc05c0.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_mc68hc05c0.h"

namespace debugger {
namespace mc68hc05c0 {

void Signals::getControl() {
#if CNTL_WRITE == CNTL_WR && CNTL_FETCH == CNTL_LIR && (CNTL_RD >> 3) == CNTL_READ
    cntl() = busRead(CNTL) ^ CNTL_LIR;
    cntl() |= (cntl() & CNTL_RD) >> 3;
#else
#error "Check CNTL_ definitions"
#endif
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
}

void Signals::inputMode() {
    busMode(AD, INPUT);
}

}  // namespace mc68hc05c0
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
