#include "dl11.h"
#include "debugger.h"

namespace debugger {
namespace pdp11 {

namespace {
constexpr uint8_t VEC_RX = 060;
constexpr uint8_t VEC_TX = 064;
// A character time at 9600 baud: received ones come no faster, as on a
// line, so pasted text does not overrun a program that reads each.
constexpr uint32_t CHAR_CYCLES = F_CPU / 960;
}  // namespace

Dl11::Dl11() : UartBase(2, VEC_RX) {
    _txIntr = VEC_TX;
}

const char *Dl11::name() const {
    return "DL11";
}

const char *Dl11::description() const {
    return "DL11";
}

void Dl11::print() const {
    const auto radix = Debugger.target().inputRadix();
    const auto digits = Debugger::numDigits(8, radix);
    cli.print("  Vector: Rx=");
    cli.printNum(_rxIntr, radix, digits);
    cli.print(" Tx=");
    cli.printNum(_txIntr, radix, digits);
    cli.println();
}

void Dl11::resetUart() {
    _rxDone = _rxIe = _rxRequest = false;
    _txIe = _txRequest = false;
    _txReady = true;
    _rxData = _txData = 0;
}

// A character is taken whether or not the program took the last one:
// left on the console, it would reach the debugger's prompt after the run.
void Dl11::loopUart() {
    if (Console.available() > 0 && ARM_DWT_CYCCNT - _rxTime >= CHAR_CYCLES) {
        _rxTime = ARM_DWT_CYCCNT;
        _rxData = Console.read();
        _rxDone = true;
        if (_rxIe)
            requestRx();
    }
    if (_txDone) {
        _txDone = false;
        _txReady = true;
        if (_txIe)
            requestTx();
    }
    if (!_txReady && Console.availableForWrite() > 0) {
        consoleWrite(_txData);
        _txDone = true;  // READY comes on the next service
    }
}

// Both requests share one line.
void Dl11::negateIntreq(uint8_t name) {
    if (vector() == 0)
        UartBase::negateIntreq(name);
}

void Dl11::requestRx() {
    _rxRequest = true;
    assertRxIntr();
}

void Dl11::cancelRx() {
    _rxRequest = false;
    negateRxIntr();
}

void Dl11::requestTx() {
    _txRequest = true;
    assertTxIntr();
}

void Dl11::cancelTx() {
    _txRequest = false;
    negateTxIntr();
}

uint16_t Dl11::acknowledge() {
    if (_rxRequest) {
        cancelRx();
        return _rxIntr;
    }
    if (_txRequest) {
        cancelTx();
        return _txIntr;
    }
    return 0;
}

// The registers' high bytes hold nothing a program writes.
void Dl11::write(uint32_t addr, uint16_t data) {
    const auto ie = (data & IE_bm) != 0;
    switch (addr - _base_addr) {
    case RCSR:
        if (ie && !_rxIe && _rxDone)
            requestRx();
        if (!ie)
            cancelRx();
        _rxIe = ie;
        break;
    case XCSR:
        if (ie && !_txIe && _txReady)
            requestTx();
        if (!ie)
            cancelTx();
        _txIe = ie;
        break;
    case XBUF:
        _txData = data;
        _txReady = false;
        cancelTx();
        break;
    }
}

uint16_t Dl11::read(uint32_t addr) {
    if (addr - _base_addr == RBUF) {
        _rxDone = false;
        cancelRx();
    }
    return peek(addr);
}

uint16_t Dl11::peek(uint32_t addr) const {
    switch (addr - _base_addr) {
    case RCSR:
        return (_rxDone ? DONE_bm : 0) | (_rxIe ? IE_bm : 0);
    case RBUF:
        return _rxData;
    case XCSR:
        return (_txReady ? DONE_bm : 0) | (_txIe ? IE_bm : 0);
    }
    return 0;
}

}  // namespace pdp11
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
