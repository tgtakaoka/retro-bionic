#include "i8085_sio_handler.h"
#include "debugger.h"
#include "pins_i8085.h"

namespace debugger {
namespace i8085 {

const char *I8085SioHandler::name() const {
    return "SIO";
}

const char *I8085SioHandler::description() const {
    return Debugger.target().cpuName();
}

uint32_t I8085SioHandler::baseAddr() const {
    return 0x00;
}

void I8085SioHandler::assert_rxd() const {
    digitalWriteFast(PIN_SID, LOW);
}

void I8085SioHandler::negate_rxd() const {
    digitalWriteFast(PIN_SID, HIGH);
}

uint8_t I8085SioHandler::signal_txd() const {
    return digitalReadFast(PIN_SOD);
}

void I8085SioHandler::resetHandler() {
    pinMode(PIN_SID, OUTPUT);
    pinMode(PIN_SOD, INPUT);
    // I8085 bitbang speed: assuming CLK is 3MHz
    _pre_divider = 24 * 2;
    _tx_divider = _rx_divider = 13;
}

}  // namespace i8085
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
