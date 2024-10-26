#include "i8051_uart_handler.h"
#include "debugger.h"
#include "pins_i8051.h"

namespace debugger {
namespace i8051 {

const char *I8051UartHandler::name() const {
    return "UART";
}

const char *I8051UartHandler::description() const {
    return Debugger.target().cpuName();
}

uint32_t I8051UartHandler::baseAddr() const {
    return 0x0099;  // SBUF
}

void I8051UartHandler::assert_rxd() const {
    digitalWriteFast(PIN_RXD, LOW);
}

void I8051UartHandler::negate_rxd() const {
    digitalWriteFast(PIN_RXD, HIGH);
}

uint8_t I8051UartHandler::signal_txd() const {
    return digitalReadFast(PIN_TXD);
}

void I8051UartHandler::resetHandler() {
    pinMode(PIN_RXD, OUTPUT);
    pinMode(PIN_TXD, INPUT);
    // I8051 UART:
    // baudrate = K*fosc/(32*12*(256-TH1)
    // 256-TH1 = K*fosc/(32*12*baudrate)
    // fosc=12MHz, K=2(SMOD=1) baudrate=4,800bps, 256-TH1=13
    _pre_divider = 12 * 13 * 2;
    _divider = 16;
}

}  // namespace i8051
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
