#include "z88_uart_handler.h"
#include "pins_z88.h"
#include "debugger.h"

namespace debugger {
namespace z88 {

const char *Z88UartHandler::name() const {
    return "UART";
}

const char *Z88UartHandler::description() const {
    return Debugger.target().cpuName();
}

uint32_t Z88UartHandler::baseAddr() const {
    return 0xEF;
}

void Z88UartHandler::assert_rxd() const {
    digitalWriteFast(PIN_RXD, LOW);
}

void Z88UartHandler::negate_rxd() const {
    digitalWriteFast(PIN_RXD, HIGH);
}

uint8_t Z88UartHandler::signal_txd() const {
    return digitalReadFast(PIN_TXD);
}

void Z88UartHandler::resetHandler() {
    pinMode(PIN_RXD, OUTPUT);
    pinMode(PIN_TXD, INPUT);
    // Z88 UART: assuming XTAL is 14.7546MHz
    // bit rate = (14,754,600 / 4) / (2 x (UBG+1) x N)
    _pre_divider = 4 * 2 * 32;  // N=32
    _divider = 11 + 1;          // UBG=11
}

}  // namespace z88
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
