#include "signals_tms99105.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_tms99105.h"

namespace debugger {
namespace tms99105 {

void SignalsTms99105::getAddress() {
    addr = busRead(ADDR);
}

void SignalsTms99105::getControl() {
    // CNTL_MEM is active low
    // CNTL_RW is active high (R/#W)
    // CNTL_IAQ is active high
    cntl() = busRead(CNTL) | CNTL_16BIT | CNTL_BST;
    rd() = digitalReadFast(PIN_RD);
    bst() = busRead(BST) | (cntl() & CNTL_MEMEN);
    if (bst() == IAQ)
        cntl() |= CNTL_IAQ;
}

bool SignalsTms99105::writeEnable() const {
    // CNTL_WE is active low
    return (cntl() & CNTL_WE) == 0;
}

bool SignalsTms99105::readEnable() const {
    // #RD is active low
    return rd() == 0;
}

bool SignalsTms99105::macrostore() const {
    return (bst() & 7) == AUMSL;
}

void SignalsTms99105::getData() {
    data = busRead(DATA);
}

void SignalsTms99105::outData() const {
    busWrite(DATA, data);
    busMode(DATA, OUTPUT);
}

void SignalsTms99105::inputMode() const {
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
