#include "uart_base.h"

#include "debugger.h"

namespace debugger {

void UartBase::reset() {
    negateRxIntr();
    negateTxIntr();
    _rxVec = _txVec = 0;
    _rxIntr = _txIntr = 0;
    _delay = 0;
    _txDone = false;
    resetUart();
}

void UartBase::loop() {
    if (_enabled && ++_delay == 0)
        loopUart();
}

void UartBase::assertRxIntr() {
    if (_rxIntr == 0)
        return;
    _rxVec = _rxIntr;
    assertIntreq(_rxIntr);
}

void UartBase::assertTxIntr() {
    if (_txIntr == 0)
        return;
    _txVec = _txIntr;
    assertIntreq(_txIntr);
}

void UartBase::negateRxIntr() {
    if (_rxIntr == 0 || _rxVec == 0)
        return;
    _rxVec = 0;
    if (_rxIntr != _txVec)
        negateIntreq(_rxIntr);
}

void UartBase::negateTxIntr() {
    if (_txIntr == 0 || _txVec == 0)
        return;
    _txVec = 0;
    if (_txIntr != _rxVec)
        negateIntreq(_txIntr);
}

void UartBase::assertIntreq(uint8_t name) {
    Debugger.target().assertInt(name);
}

void UartBase::negateIntreq(uint8_t name) {
    Debugger.target().negateInt(name);
}

void UartBase::write(uint32_t addr, uint16_t data) {
    if (addr == _base_addr + 2 * _step) {
        _rxIntr = data;
    } else if (addr == _base_addr + 3 * _step) {
        _txIntr = data;
    }
}

uint16_t UartBase::read(uint32_t addr) {
    if (addr == _base_addr + 2 * _step) {
        return _rxIntr;
    } else if (addr == _base_addr + 3 * _step) {
        return _txIntr;
    }
    return 0;
}

}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
