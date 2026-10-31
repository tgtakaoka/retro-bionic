#include "h8300_sci_handler.h"
#include "debugger.h"
#include "pins_h8300.h"

namespace debugger {
namespace h8300 {

const char *H8300SciHandler::name() const {
    return "SCI";
}

const char *H8300SciHandler::description() const {
    return Debugger.target().cpuName();
}

// SCI0's SMR (H8/325 Series Hardware Manual Table 9-2).
uint32_t H8300SciHandler::baseAddr() const {
    return 0xFFD8;
}

void H8300SciHandler::assert_rxd() const {
    digitalWriteFast(PIN_RXD, LOW);
}

void H8300SciHandler::negate_rxd() const {
    digitalWriteFast(PIN_RXD, HIGH);
}

uint8_t H8300SciHandler::signal_txd() const {
    return digitalReadFast(PIN_TXD);
}

void H8300SciHandler::resetHandler() {
    pinMode(PIN_RXD, OUTPUT);
    pinMode(PIN_TXD, INPUT);
    // A bit is 32 x (BRR + 1) ø clocks from the internal clock with n = 0
    // (9.2.8); samples/h8300 sets BRR to 12, 416 ø clocks a bit.
    _pre_divider = 26;
    _tx_divider = _rx_divider = 16;
}

}  // namespace h8300
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
