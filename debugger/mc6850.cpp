#include <Arduino.h>

#include "debugger.h"
#include "mc6850.h"

namespace debugger {

struct Mc6850 ACIA;

const char *Mc6850::name() const {
    return "ACIA";
}

const char *Mc6850::description() const {
    return "MC6850";
}

void Mc6850::print() const {
    // RX and TX share the same interrupt
    cli.print("  #IRQ: ");
    cli.printlnDec(_rxIntr);
}

void Mc6850::resetUart() {
    // RX and TX share the same interrupt
    _rxIntr = _txIntr = 1;
    _control = CDS_DIV1_gc;
    _status = TDRE_bm;
    _txData = _rxData = 0;
}

void Mc6850::loopUart() {
    if (Console.available() > 0) {
        _rxData = Console.read();
        if (rxReady())
            _status |= OVRN_bm;
        _status |= RDRF_bm;
        if (rxIntEnabled())
            assertRxIntr();
    }
    // TODO: Implement flow control
    if (Console.availableForWrite() > 0 && !txReady()) {
        Console.write(_txData);
        _status |= TDRE_bm;
        if (txIntEnabled())
            assertTxIntr();
    }
}

void Mc6850::assertIntreq(uint8_t name) {
    _status |= IRQF_bm;
    UartBase::assertIntreq(name);
}

void Mc6850::negateIntreq(uint8_t name) {
    UartBase::negateIntreq(name);
    _status &= ~IRQF_bm;
}

void Mc6850::write(uint32_t addr, uint16_t data) {
    if (addr == _base_addr) {
        _control = data;
        if (masterReset()) {
            negateRxIntr();
            negateTxIntr();
            _status = TDRE_bm;
        }
        // TODO: flow control
        if (txReady() && txIntEnabled()) {
            assertTxIntr();
        } else {
            negateTxIntr();
        }
        if (rxReady() && rxIntEnabled()) {
            assertRxIntr();
        } else {
            negateRxIntr();
        }
    } else if (addr == _base_addr + 1U) {
        _txData = data;
        _status &= ~TDRE_bm;
        negateTxIntr();
    } else {
        // RX and TX share the same interrupt
        _rxIntr = _txIntr = data;
    }
}

uint16_t Mc6850::read(uint32_t addr) {
    if (addr == _base_addr)
        return _status;
    if (addr == _base_addr + 1U) {
        _status &= ~(RDRF_bm | OVRN_bm);
        negateRxIntr();
        return _rxData;
    }
    // RX and TX share the same interrupt
    return _rxIntr;
}

}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
