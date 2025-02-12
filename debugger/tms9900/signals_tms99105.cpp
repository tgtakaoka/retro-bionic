#include "signals_tms99105.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_tms99105.h"

namespace debugger {
namespace tms99105 {

void Signals::getAddress() {
    addr = busRead(ADDR);
}

void Signals::getControl() {
    // CNTL_MEM is active low
    // CNTL_RW is active high (R/#W)
    // CNTL_IAQ is active high
    cntl() = busRead(CNTL) | CNTL_16BIT;
    rd() = digitalReadFast(PIN_RD);
    bst() = busRead(BST) | (cntl() & CNTL_MEMEN);
    if (bst() == IAQ)
        cntl() |= CNTL_IAQ;
}

bool Signals::writeEnable() const {
    // CNTL_WE is active low
    return (cntl() & CNTL_WE) == 0;
}

bool Signals::readEnable() const {
    // #RD is active low
    return rd() == 0;
}

bool Signals::macrostore() const {
    return (bst() & 7) == AUMSL;
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

}  // namespace tms99105
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
