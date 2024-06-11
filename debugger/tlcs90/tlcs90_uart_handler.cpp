#include "tlcs90_uart_handler.h"
#include "debugger.h"
#include "pins_tlcs90.h"
#include "regs_tlcs90.h"

namespace debugger {
namespace tlcs90 {

const char *Tlcs90UartHandler::name() const {
    return "UART";
}

const char *Tlcs90UartHandler::description() const {
    return Regs.cpuName();
}

uint32_t Tlcs90UartHandler::baseAddr() const {
    return 0xFFEB;
}

void Tlcs90UartHandler::assert_rxd() const {
    digitalWriteFast(PIN_RXD, LOW);
}

void Tlcs90UartHandler::negate_rxd() const {
    digitalWriteFast(PIN_RXD, HIGH);
}

uint8_t Tlcs90UartHandler::signal_txd() const {
    return digitalReadFast(PIN_TXD);
}

void Tlcs90UartHandler::resetHandler() {
    pinMode(PIN_RXD, OUTPUT);
    pinMode(PIN_TXD, INPUT);
    // TLCS90 UART: assuming XTAL is fc=9.8304MHz
    // bitrate=fc/4/Tn/SC/16
    // TRUN_BRATE=11, T4 clock, 9600bps
    // SCMOD_SC=11, 1/2 division
    _pre_divider = 4 * 4 * 2;
    _divider = 16;
}

}  // namespace tlcs90
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
